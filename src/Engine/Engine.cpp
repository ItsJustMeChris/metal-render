#include "Engine.hpp"
#include "imgui.h"
#include "backends/imgui_impl_metal.h"
#include "backends/imgui_impl_sdl2.h"

#define MTL_DEBUG_LAYER 1

Engine::Engine(const std::string &title)
    : window(nullptr, SDL_DestroyWindow),
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
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return;
    }

    window.reset(SDL_CreateWindow(title.c_str(),
                                  SDL_WINDOWPOS_CENTERED,
                                  SDL_WINDOWPOS_CENTERED,
                                  1920,
                                  1080,
                                  SDL_WINDOW_METAL | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE));

    if (!window)
    {
        std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
        return;
    }

    metalView = SDL_Metal_CreateView(window.get());
    if (!metalView)
    {
        std::cerr << "SDL_Metal_CreateView Error: " << SDL_GetError() << std::endl;
        return;
    }

    SDL_AddEventWatch([](void *userdata, SDL_Event *event) -> int
                      {
        if (event->type == SDL_WINDOWEVENT && event->window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
        {
            Engine *engine = static_cast<Engine*>(userdata);
            engine->resizeFrameBuffer();
        }
        return 1; },
                      this);

    camera = Camera({0.0f, 0.0f, 1.0f}, {0.0f, 1.0f, 0.0f}, -90.0f, 0.0f);

    InitMetal();

    running = true;
}

void Engine::resizeFrameBuffer()
{
    CA::MetalLayer *metalLayer = (CA::MetalLayer *)SDL_Metal_GetLayer(metalView);
    metalLayer->setDrawableSize(metalLayer->drawableSize());

    if (msaaRenderTargetTexture)
    {
        msaaRenderTargetTexture->release();
        msaaRenderTargetTexture = nullptr;
    }
    if (depthTexture)
    {
        depthTexture->release();
        depthTexture = nullptr;
    }

    createDepthAndMSAATextures();

    if (metalDrawable)
    {
        metalDrawable->release();
        metalDrawable = nullptr;
    }
}

Engine::~Engine()
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

    ImGui_ImplMetal_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    if (metalView)
        SDL_Metal_DestroyView(metalView);

    SDL_Quit();
}

void Engine::InitMetal()
{
    CA::MetalLayer *metalLayer = static_cast<CA::MetalLayer *>(SDL_Metal_GetLayer(metalView));

    device = MTL::CreateSystemDefaultDevice();

    metalLayer->setDevice(device);
    metalLayer->setPixelFormat(MTL::PixelFormatBGRA8Unorm);

    // Assume running from root directory and not from build directory
    renderables.push_back(std::make_unique<Renderable>(device, "bin/Release/assets/teapot.obj", glm::vec3(0.0f, 0.0f, 0.0f)));
    renderables.push_back(std::make_unique<Renderable>(device, "bin/Release/assets/teapot.obj", glm::vec3(10.0f, 0.0f, 0.0f)));
    renderables.push_back(std::make_unique<Renderable>(device, "bin/Release/assets/cow.obj", glm::vec3(0.0f, 50.0f, 0.0f)));
    renderables.push_back(std::make_unique<Renderable>(device, "bin/Release/assets/teddy.obj", glm::vec3(50.0f, 50.0f, 0.0f)));

    createDefaultLibrary();
    createCommandQueue();
    createRenderPipeline();
    createDepthAndMSAATextures();

    LightData lightData;
    lightData.ambientColor = simd::float3{0.2f, 0.2f, 0.2f};
    lightData.lightDirection = simd::float3{1.0f, 1.0f, 1.0f};
    lightData.lightColor = simd::float3{1.0f, 1.0f, 1.0f};

    lightBuffer.reset(device->newBuffer(&lightData, sizeof(LightData), MTL::ResourceStorageModeShared));

    renderPassDescriptor.reset(MTL::RenderPassDescriptor::alloc()->init());

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    ImGui::StyleColorsDark();

    ImGui_ImplMetal_Init(device);
    ImGui_ImplSDL2_InitForMetal(window.get());
}

void Engine::createDefaultLibrary()
{
    metalDefaultLibrary = device->newDefaultLibrary();

    if (!metalDefaultLibrary)
    {
        std::cerr << "Failed to load default library." << std::endl;
        if (device)
            device->release();
        std::exit(-1);
    }
}

void Engine::createCommandQueue()
{
    metalCommandQueue = device->newCommandQueue();
    if (!metalCommandQueue)
    {
        std::cerr << "Failed to create command queue.";
        std::exit(-1);
    }
}

void Engine::createRenderPipeline()
{
    MTL::Function *vertexShader = metalDefaultLibrary->newFunction(NS::String::string("geometry_VertexShader", NS::ASCIIStringEncoding));
    assert(vertexShader);
    MTL::Function *fragmentShader = metalDefaultLibrary->newFunction(NS::String::string("geometry_FragmentShader", NS::ASCIIStringEncoding));
    assert(fragmentShader);

    MTL::RenderPipelineDescriptor *renderPipelineDescriptor = MTL::RenderPipelineDescriptor::alloc()->init();
    renderPipelineDescriptor->setLabel(NS::String::string("Square Rendering Pipeline", NS::ASCIIStringEncoding));
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

void Engine::createDepthAndMSAATextures()
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

void Engine::draw()
{
    sendRenderCommand();
}

void Engine::drawImGui(MTL::RenderPassDescriptor *renderPassDescriptor)
{
    float aspectRatio = static_cast<float>(metalDrawable->texture()->width()) /
                        static_cast<float>(metalDrawable->texture()->height());

    MTL::RenderCommandEncoder *imguiRenderCommandEncoder =
        metalCommandBuffer->renderCommandEncoder(renderPassDescriptor);

    ImGui_ImplMetal_NewFrame(renderPassDescriptor);
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    ImGuiIO &io = ImGui::GetIO();
    float dpiScaleFactor = static_cast<float>(metalDrawable->texture()->width()) / io.DisplaySize.x;

    static bool showDemoWindow = false;
    if (showDemoWindow)
        ImGui::ShowDemoWindow(&showDemoWindow);

    ImGui::Begin("Renderables");

    int index = 0;
    ImDrawList *drawList = ImGui::GetBackgroundDrawList();

    for (auto &renderable : renderables)
    {
        std::string renderableName = "Renderable " + std::to_string(index);

        if (ImGui::CollapsingHeader(renderableName.c_str()))
        {
            glm::vec4 viewport(0, 0, metalDrawable->texture()->width(),
                               metalDrawable->texture()->height());

            glm::vec2 screenPos = camera.WorldToScreen(
                renderable->getPosition(),
                camera.GetProjectionMatrix(aspectRatio),
                camera.GetViewMatrix(),
                viewport);

            bool isInFrontOfCamera =
                (screenPos.x != -FLT_MAX && screenPos.y != -FLT_MAX);

            screenPos /= dpiScaleFactor;

            if (isInFrontOfCamera)
            {
                drawList->AddLine(ImVec2(io.DisplaySize.x / 2, io.DisplaySize.y / 2),
                                  ImVec2(screenPos.x, screenPos.y),
                                  IM_COL32(255, 255, 255, 255), 2.0f);

                drawList->AddText(ImVec2(screenPos.x, screenPos.y),
                                  IM_COL32(255, 255, 255, 255),
                                  renderableName.c_str());
            }

            ImGui::Text("Position: (%.2f, %.2f, %.2f)",
                        renderable->getPosition().x,
                        renderable->getPosition().y,
                        renderable->getPosition().z);
            ImGui::Text("Status: %s",
                        isInFrontOfCamera ? "In front of the camera"
                                          : "Behind the camera or invalid");

            if (ImGui::Button(("Teleport##" + std::to_string(index)).c_str()))
            {
                camera.Teleport(renderable->getPosition());
            }
            ImGui::SameLine();
            if (ImGui::Button(("Look At##" + std::to_string(index)).c_str()))
            {
                camera.LookAt(renderable->getPosition());
            }
        }
        index++;
    }

    ImGui::End();

    ImGui::Begin("Camera Controls");

    ImGui::Text("Screen Size: (%lu, %lu)",
                metalDrawable->texture()->width(),
                metalDrawable->texture()->height());
    ImGui::Text("ImGui Display Size: (%.0f, %.0f)",
                io.DisplaySize.x, io.DisplaySize.y);
    ImGui::Text("Camera Position: (%.2f, %.2f, %.2f)",
                camera.GetPosition().x,
                camera.GetPosition().y,
                camera.GetPosition().z);
    ImGui::Text("DPI Scale Factor: %.2f", dpiScaleFactor);

    static float teleportCoords[3] = {490.0f, -281.0f, -4387.0f};
    ImGui::InputFloat3("Teleport Coordinates", teleportCoords);

    if (ImGui::Button("Teleport##Camera"))
    {
        camera.Teleport(glm::vec3(teleportCoords[0], teleportCoords[1], teleportCoords[2]));
    }

    ImGui::End();

    ImGui::Render();
    ImDrawData *drawData = ImGui::GetDrawData();
    ImGui_ImplMetal_RenderDrawData(drawData, metalCommandBuffer.get(), imguiRenderCommandEncoder);

    imguiRenderCommandEncoder->endEncoding();
    imguiRenderCommandEncoder->release();
}

simd::float3 glmToSimd(const glm::vec3 &v)
{
    return simd::float3{v.x, v.y, v.z};
}

void Engine::sendRenderCommand()
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

    drawRenderables(renderCommandEncoder);

    renderCommandEncoder->endEncoding();
    renderCommandEncoder->release();

    cd->setLoadAction(MTL::LoadActionLoad);

    drawImGui(renderPassDescriptor.get());

    metalCommandBuffer->presentDrawable(metalDrawable);

    metalCommandBuffer->commit();
}

void Engine::drawRenderables(MTL::RenderCommandEncoder *renderCommandEncoder)
{
    CA::MetalLayer *metalLayer = static_cast<CA::MetalLayer *>(SDL_Metal_GetLayer(metalView));

    for (const auto &renderable : renderables)
    {
        renderable->draw(metalLayer, camera, renderCommandEncoder, metalRenderPSO, depthStencilState);
    }
}

void Engine::Run()
{
    Uint64 NOW = SDL_GetPerformanceCounter();
    Uint64 LAST = 0;
    double deltaTime = 0;

    // This effectively caps the framerate to 120 FPS (Change this to 144 for 144 FPS, etc.)
    const double fixedTimeStep = 1.0 / 120.0; // 120 is the macbook pro "pro motion" refresh rate
    double accumulator = 0.0;

    int windowWidth, windowHeight;
    SDL_GetWindowSize(window.get(), &windowWidth, &windowHeight);

    while (running)
    {
        LAST = NOW;
        NOW = SDL_GetPerformanceCounter();
        deltaTime = (double)((NOW - LAST) / (double)SDL_GetPerformanceFrequency());
        accumulator += deltaTime;

        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL2_ProcessEvent(&event);

            if (event.type == SDL_QUIT)
                running = false;

            if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT)
            {
                if (!ImGui::GetIO().WantCaptureMouse)
                {
                    int x, y;
                    SDL_GetMouseState(&x, &y);
                    camera.OnMouseButtonDown(x, y);
                }
            }
            else if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT)
            {
                camera.OnMouseButtonUp();
            }
        }

        if (!ImGui::GetIO().WantCaptureMouse)
        {
            int mouseX, mouseY;
            SDL_GetMouseState(&mouseX, &mouseY);
            camera.OnMouseMove(mouseX, mouseY);
        }

        if (!ImGui::GetIO().WantCaptureKeyboard)
        {
            const Uint8 *state = SDL_GetKeyboardState(NULL);
            camera.ProcessKeyboardInput(state, (float)deltaTime);
        }

        while (accumulator >= fixedTimeStep)
        {
            accumulator -= fixedTimeStep;
        }

        draw();
    }
}
