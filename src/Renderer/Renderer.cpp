#include "Renderer.hpp"
#include "ImGuiHandler.hpp"

Renderer::Renderer(SDL_MetalView metalView)
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
    initMetal();
}

Renderer::~Renderer()
{
    msaaRenderTargetTexture.reset();
    depthTexture.reset();
    renderPassDescriptor.reset();
    lightBuffer.reset();

    if (metalDefaultLibrary)
        metalDefaultLibrary->release();

    if (metalCommandQueue)
        metalCommandQueue->release();

    if (metalRenderPSO)
        metalRenderPSO->release();

    if (depthStencilState)
        depthStencilState->release();

    if (device)
        device->release();

    renderables.clear();
}

void Renderer::initMetal()
{
    CA::MetalLayer *metalLayer = static_cast<CA::MetalLayer *>(SDL_Metal_GetLayer(metalView));

    device = MTL::CreateSystemDefaultDevice();

    metalLayer->setDevice(device);
    metalLayer->setPixelFormat(MTL::PixelFormatBGRA8Unorm);

    // Load renderables
    renderables.push_back(std::make_unique<Renderable>(device, "bin/Release/assets/teapot.obj", glm::vec3(0.0f, 0.0f, 0.0f)));
    renderables.push_back(std::make_unique<Renderable>(device, "bin/Release/assets/teapot.obj", glm::vec3(10.0f, 0.0f, 0.0f)));
    renderables.push_back(std::make_unique<Renderable>(device, "bin/Release/assets/cow.obj", glm::vec3(0.0f, 50.0f, 0.0f)));
    renderables.push_back(std::make_unique<Renderable>(device, "bin/Release/assets/teddy.obj", glm::vec3(50.0f, 50.0f, 0.0f)));

    // Create pipeline and buffers
    createRenderPipeline();
    createDepthAndMSAATextures();

    LightData lightData;
    lightData.ambientColor = simd::float3{0.2f, 0.2f, 0.2f};
    lightData.lightDirection = simd::float3{1.0f, 1.0f, 1.0f};
    lightData.lightColor = simd::float3{1.0f, 1.0f, 1.0f};

    lightBuffer.reset(device->newBuffer(&lightData, sizeof(LightData), MTL::ResourceStorageModeShared));

    renderPassDescriptor.reset(MTL::RenderPassDescriptor::alloc()->init());

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

void Renderer::createRenderPipeline()
{
    // Create default library
    metalDefaultLibrary = device->newDefaultLibrary();

    if (!metalDefaultLibrary)
    {
        std::cerr << "Failed to load default library." << std::endl;
        if (device)
            device->release();
        std::exit(-1);
    }

    // Create command queue
    metalCommandQueue = device->newCommandQueue();
    if (!metalCommandQueue)
    {
        std::cerr << "Failed to create command queue.";
        std::exit(-1);
    }

    // Create render pipeline
    MTL::Function *vertexShader = metalDefaultLibrary->newFunction(NS::String::string("geometry_VertexShader", NS::ASCIIStringEncoding));
    assert(vertexShader);
    MTL::Function *fragmentShader = metalDefaultLibrary->newFunction(NS::String::string("geometry_FragmentShader", NS::ASCIIStringEncoding));
    assert(fragmentShader);

    MTL::RenderPipelineDescriptor *renderPipelineDescriptor = MTL::RenderPipelineDescriptor::alloc()->init();
    renderPipelineDescriptor->setLabel(NS::String::string("Rendering Pipeline", NS::ASCIIStringEncoding));
    renderPipelineDescriptor->setVertexFunction(vertexShader);
    renderPipelineDescriptor->setFragmentFunction(fragmentShader);
    assert(renderPipelineDescriptor);

    CA::MetalLayer *metalLayer = (CA::MetalLayer *)SDL_Metal_GetLayer(metalView);
    MTL::PixelFormat pixelFormat = (MTL::PixelFormat)metalLayer->pixelFormat();
    renderPipelineDescriptor->colorAttachments()->object(0)->setPixelFormat(pixelFormat);
    renderPipelineDescriptor->setSampleCount(sampleCount);
    renderPipelineDescriptor->setDepthAttachmentPixelFormat(MTL::PixelFormatDepth32Float);

    NS::Error *error;
    metalRenderPSO = device->newRenderPipelineState(renderPipelineDescriptor, &error);

    if (!metalRenderPSO)
    {
        std::cerr << "Failed to create render pipeline state: " << error->localizedDescription()->utf8String() << std::endl;
        std::exit(-1);
    }

    MTL::DepthStencilDescriptor *depthStencilDescriptor = MTL::DepthStencilDescriptor::alloc()->init();
    depthStencilDescriptor->setDepthCompareFunction(MTL::CompareFunctionLess);
    depthStencilDescriptor->setDepthWriteEnabled(true);
    depthStencilState = device->newDepthStencilState(depthStencilDescriptor);

    renderPipelineDescriptor->release();
    vertexShader->release();
    fragmentShader->release();
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

    renderCommandEncoder->setFragmentBuffer(lightBuffer.get(), 0, 1);

    drawRenderables(renderCommandEncoder, camera);

    renderCommandEncoder->endEncoding();
    renderCommandEncoder->release();

    MTL::RenderPassColorAttachmentDescriptor *nd = renderPassDescriptor->colorAttachments()->object(0);
    nd->setLoadAction(MTL::LoadActionLoad);

    imguiHandler.render(metalCommandBuffer.get(), renderPassDescriptor.get(), metalDrawable->texture()->width(), metalDrawable->texture()->height());

    metalCommandBuffer->presentDrawable(metalDrawable);
    metalCommandBuffer->commit();
}

void Renderer::drawRenderables(MTL::RenderCommandEncoder *renderCommandEncoder, Camera &camera)
{
    CA::MetalLayer *metalLayer = static_cast<CA::MetalLayer *>(SDL_Metal_GetLayer(metalView));

    for (const auto &renderable : renderables)
    {
        renderable->draw(metalLayer, camera, renderCommandEncoder, metalRenderPSO, depthStencilState);
    }
}
