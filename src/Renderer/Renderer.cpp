#include "Renderer.hpp"
#include "Engine.hpp"
#include "ImGuiHandler.hpp"

Renderer::Renderer(SDL_MetalView metalView, Engine *engine)
    : metalView(metalView),
      metalCommandBuffer(nullptr, [](MTL::CommandBuffer *b)
                         { if(b) b->release(); }),
      msaaRenderTargetTexture(nullptr, [](MTL::Texture *t)
                              { if(t) t->release(); }),
      depthTexture(nullptr, [](MTL::Texture *t)
                   { if(t) t->release(); }),
      renderPassDescriptor(nullptr, [](MTL::RenderPassDescriptor *r)
                           { if(r) r->release(); }),
      lightBuffer(nullptr, [](MTL::Buffer *b)
                  { if(b) b->release(); })
{
    this->engine = engine;
    initMetal();
}

Renderer::~Renderer()
{
    msaaRenderTargetTexture.reset();
    depthTexture.reset();
    renderPassDescriptor.reset();
    lightBuffer.reset();

    if (metalCommandQueue)
        metalCommandQueue->release();

    if (depthStencilState)
        depthStencilState->release();

    if (device)
        device->release();

    renderables.clear();

    delete pipelineManager;
}

glm::vec3 Renderer::TraceLine(const glm::vec3 &origin, const glm::vec3 &destination)
{
    // Cast a ray between the origin and destination and test if it intersects with any objects in the scene (renderables) and return the intersection point if it does
    for (const auto &renderable : renderables)
    {
        if (auto intersection = renderable->intersect(origin, destination))
        {
            printf("Intersection found at (%f, %f, %f)\n", intersection.value().x, intersection.value().y, intersection.value().z);
            return intersection.value();
        }
    }

    return glm::vec3(0.0f, 0.0f, 0.0f);
}

void Renderer::initMetal()
{
    CA::MetalLayer *metalLayer = static_cast<CA::MetalLayer *>(SDL_Metal_GetLayer(metalView));

    device = MTL::CreateSystemDefaultDevice();

    pipelineManager = new PipelineManager(device);
    pipelineManager->engine = engine;

    metalLayer->setDevice(device);
    metalLayer->setPixelFormat(MTL::PixelFormatBGRA8Unorm);

    createRenderPipelines();
    createDepthAndMSAATextures();

    lightData = {};
    lightData.ambientColor = simd::float3{0.1f, 0.1f, 0.1f};
    lightData.lightColor = simd::float3{1.f, 1.f, 1.f};

    lightBuffer.reset(device->newBuffer(sizeof(LightData), MTL::ResourceStorageModeShared));

    renderPassDescriptor.reset(MTL::RenderPassDescriptor::alloc()->init());

    auto teapotModel = std::make_shared<Model>(device, "bin/Release/assets/teapot.obj");
    auto cowModel = std::make_shared<Model>(device, "bin/Release/assets/cow.obj");
    auto teddyModel = std::make_shared<Model>(device, "bin/Release/assets/teddy.obj");
    auto capsuleModel = std::make_shared<Model>(device, "bin/Release/assets/capsule/capsule.obj");
    auto smgModel = std::make_shared<Model>(device, "bin/Release/assets/SMG/smg.obj");
    auto backpackModel = std::make_shared<Model>(device, "bin/Release/assets/backpack/backpack.obj");
    auto sunModel = std::make_shared<Model>(device, "bin/Release/assets/Beach_Ball_v2_L3.123cdf1ec704-c7ca-4faf-8f47-647b6e5df698/13517_Beach_Ball_v2_L3.obj");

    renderables.push_back(std::make_unique<Renderable>(device, this->engine, pipelineManager, "standard", teapotModel, glm::vec3(0.0f, 0.0f, 0.0f)));
    renderables.push_back(std::make_unique<Renderable>(device, this->engine, pipelineManager, "standard", teapotModel, glm::vec3(10.0f, 0.0f, 0.0f)));
    renderables.push_back(std::make_unique<Renderable>(device, this->engine, pipelineManager, "standard", capsuleModel, glm::vec3(10.0f, 10.0f, 0.0f)));
    renderables.push_back(std::make_unique<Renderable>(device, this->engine, pipelineManager, "standard", smgModel, glm::vec3(10.0f, 10.0f, 10.0f)));
    renderables.push_back(std::make_unique<Renderable>(device, this->engine, pipelineManager, "standard", backpackModel, glm::vec3(0.0f, 20.0f, 10.0f)));
    // renderables.push_back(std::make_unique<Renderable>(device, this->engine, pipelineManager, "debug", cowModel, glm::vec3(0.0f, 50.0f, 0.0f)));
    // renderables.push_back(std::make_unique<Renderable>(device, this->engine, pipelineManager, "debug", teddyModel, glm::vec3(50.0f, 50.0f, 0.0f)));

    auto sun = std::make_unique<Renderable>(
        device, this->engine, pipelineManager, "standard", sunModel,
        glm::vec3(30.0f, 30.0f, 0.0f), "Sun");

    sunRenderable = sun.get();
    renderables.push_back(std::move(sun));

    setupEventHandlers();
}

void Renderer::setupEventHandlers()
{
    SDL_AddEventWatch([](void *userdata, SDL_Event *event) -> int
                      {
        if (event->type == SDL_WINDOWEVENT && event->window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
        {
            Renderer* renderer = static_cast<Renderer*>(userdata);
            renderer->resizeDrawable();
        }
        return 1; }, this);
}

void Renderer::resizeDrawable()
{
    CA::MetalLayer *metalLayer = static_cast<CA::MetalLayer *>(SDL_Metal_GetLayer(metalView));
    metalLayer->setDrawableSize(metalLayer->drawableSize());

    msaaRenderTargetTexture.reset();
    depthTexture.reset();

    createDepthAndMSAATextures();

    if (metalDrawable)
    {
        metalDrawable->release();
        metalDrawable = nullptr;
    }
}

void Renderer::createRenderPipelines()
{
    // Create command queue
    metalCommandQueue = device->newCommandQueue();
    if (!metalCommandQueue)
    {
        std::cerr << "Failed to create command queue.";
        std::exit(-1);
    }

    // Create a base render pipeline descriptor with common settings
    MTL::RenderPipelineDescriptor *renderPipelineDescriptor = MTL::RenderPipelineDescriptor::alloc()->init();
    renderPipelineDescriptor->setLabel(NS::String::string("Base Rendering Pipeline", NS::ASCIIStringEncoding));

    CA::MetalLayer *metalLayer = static_cast<CA::MetalLayer *>(SDL_Metal_GetLayer(metalView));
    MTL::PixelFormat pixelFormat = metalLayer->pixelFormat();
    renderPipelineDescriptor->colorAttachments()->object(0)->setPixelFormat(pixelFormat);
    renderPipelineDescriptor->setSampleCount(sampleCount);
    renderPipelineDescriptor->setDepthAttachmentPixelFormat(MTL::PixelFormatDepth32Float);

    {
        printf("Creating standard pipeline\n");
        MTL::Function *vertexShader = pipelineManager->library->newFunction(NS::String::string("geometry_VertexShader", NS::ASCIIStringEncoding));
        MTL::Function *fragmentShader = pipelineManager->library->newFunction(NS::String::string("geometry_FragmentShader", NS::ASCIIStringEncoding));

        renderPipelineDescriptor->setVertexFunction(vertexShader);
        renderPipelineDescriptor->setFragmentFunction(fragmentShader);

        pipelineManager->createPipeline("standard", renderPipelineDescriptor);

        vertexShader->release();
        fragmentShader->release();
    }

    {
        MTL::Function *vertexShader = pipelineManager->library->newFunction(NS::String::string("debug_geometry_VertexShader", NS::ASCIIStringEncoding));
        MTL::Function *fragmentShader = pipelineManager->library->newFunction(NS::String::string("debug_geometry_FragmentShader", NS::ASCIIStringEncoding));

        renderPipelineDescriptor->setVertexFunction(vertexShader);
        renderPipelineDescriptor->setFragmentFunction(fragmentShader);

        pipelineManager->createPipeline("debug", renderPipelineDescriptor);

        vertexShader->release();
        fragmentShader->release();
    }

    renderPipelineDescriptor->release();

    MTL::DepthStencilDescriptor *depthStencilDescriptor = MTL::DepthStencilDescriptor::alloc()->init();
    depthStencilDescriptor->setDepthCompareFunction(MTL::CompareFunctionLess);
    depthStencilDescriptor->setDepthWriteEnabled(true);
    depthStencilState = device->newDepthStencilState(depthStencilDescriptor);
    depthStencilDescriptor->release();
}

void Renderer::createDepthAndMSAATextures()
{
    CA::MetalLayer *metalLayer = static_cast<CA::MetalLayer *>(SDL_Metal_GetLayer(metalView));

    MTL::TextureDescriptor *msaaTextureDescriptor = MTL::TextureDescriptor::alloc()->init();
    msaaTextureDescriptor->setTextureType(MTL::TextureType2DMultisample);
    msaaTextureDescriptor->setPixelFormat(MTL::PixelFormatBGRA8Unorm);
    msaaTextureDescriptor->setWidth(metalLayer->drawableSize().width);
    msaaTextureDescriptor->setHeight(metalLayer->drawableSize().height);
    msaaTextureDescriptor->setSampleCount(sampleCount);
    msaaTextureDescriptor->setUsage(MTL::TextureUsageRenderTarget);

    msaaRenderTargetTexture.reset(device->newTexture(msaaTextureDescriptor));

    MTL::TextureDescriptor *depthTextureDescriptor = MTL::TextureDescriptor::alloc()->init();
    depthTextureDescriptor->setTextureType(MTL::TextureType2DMultisample);
    depthTextureDescriptor->setPixelFormat(MTL::PixelFormatDepth32Float);
    depthTextureDescriptor->setWidth(metalLayer->drawableSize().width);
    depthTextureDescriptor->setHeight(metalLayer->drawableSize().height);
    depthTextureDescriptor->setUsage(MTL::TextureUsageRenderTarget);
    depthTextureDescriptor->setSampleCount(sampleCount);

    depthTexture.reset(device->newTexture(depthTextureDescriptor));

    msaaTextureDescriptor->release();
    depthTextureDescriptor->release();
}

void Renderer::render(Camera &camera, ImGuiHandler &imguiHandler)
{
    CA::MetalLayer *metalLayer = static_cast<CA::MetalLayer *>(SDL_Metal_GetLayer(metalView));

    metalDrawable = metalLayer->nextDrawable();
    if (!metalDrawable)
    {
        std::cerr << "Failed to get next drawable, skipping frame." << std::endl;
        return;
    }

    metalCommandBuffer.reset(metalCommandQueue->commandBuffer());

    MTL::RenderPassColorAttachmentDescriptor *cd = renderPassDescriptor->colorAttachments()->object(0);
    cd->setTexture(metalDrawable->texture());
    cd->setLoadAction(MTL::LoadActionClear);
    cd->setClearColor(MTL::ClearColor(41.0f / 255.0f, 42.0f / 255.0f, 48.0f / 255.0f, 1.0f));
    cd->setStoreAction(MTL::StoreActionStore);

    renderPassDescriptor->depthAttachment()->setTexture(depthTexture.get());
    renderPassDescriptor->depthAttachment()->setClearDepth(1.0);
    renderPassDescriptor->depthAttachment()->setLoadAction(MTL::LoadActionClear);
    renderPassDescriptor->depthAttachment()->setStoreAction(MTL::StoreActionDontCare);

    MTL::RenderCommandEncoder *renderCommandEncoder = metalCommandBuffer->renderCommandEncoder(renderPassDescriptor.get());

    renderCommandEncoder->setViewport(MTL::Viewport{0.0, 0.0,
                                                    static_cast<double>(metalDrawable->texture()->width()),
                                                    static_cast<double>(metalDrawable->texture()->height()),
                                                    0.0, 1.0});

    renderCommandEncoder->setDepthStencilState(depthStencilState);

    if (sunRenderable)
    {
        glm::vec3 sunPos = sunRenderable->getPosition();
        lightData.lightPosition = simd::float3{sunPos.x, sunPos.y, sunPos.z};
    }

    memcpy(lightBuffer->contents(), &lightData, sizeof(LightData));

    renderCommandEncoder->setFragmentBuffer(lightBuffer.get(), 0, 1);

    drawRenderables(renderCommandEncoder, camera);

    renderCommandEncoder->endEncoding();
    renderCommandEncoder->release();

    MTL::RenderPassColorAttachmentDescriptor *nd = renderPassDescriptor->colorAttachments()->object(0);
    nd->setLoadAction(MTL::LoadActionLoad);

    imguiHandler.render(metalCommandBuffer.get(), renderPassDescriptor.get());

    metalCommandBuffer->presentDrawable(metalDrawable);
    metalCommandBuffer->commit();
}

void Renderer::drawRenderables(MTL::RenderCommandEncoder *renderCommandEncoder, Camera &camera)
{
    for (const auto &renderable : renderables)
    {
        renderable->draw(camera, renderCommandEncoder, depthStencilState);
    }
}
