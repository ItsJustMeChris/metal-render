#include "Engine.hpp"
#include "imgui.h"
#include "backends/imgui_impl_metal.h"
#include "backends/imgui_impl_sdl2.h"

#define MTL_DEBUG_LAYER 1

// Constructor
Engine::Engine(const std::string &title)
{
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return;
    }

    // Create SDL Window with Metal support
    window = SDL_CreateWindow(title.c_str(),
                              SDL_WINDOWPOS_CENTERED,
                              SDL_WINDOWPOS_CENTERED,
                              1920,
                              1080,
                              SDL_WINDOW_METAL | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE);

    if (!window)
    {
        std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
        return;
    }

    // Create Metal view
    metalView = SDL_Metal_CreateView(window);
    if (!metalView)
    {
        std::cerr << "SDL_Metal_CreateView Error: " << SDL_GetError() << std::endl;
        return;
    }

    // sdl window size event handler
    SDL_AddEventWatch([](void *userdata, SDL_Event *event) -> int
                      {
        if (event->type == SDL_WINDOWEVENT && event->window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
        {
            Engine *engine = (Engine *)userdata;
            engine->resizeFrameBuffer();
        }
        return 1; },
                      this);

    camera = Camera({0.0f, 0.0f, 1.0f}, {0.0f, 1.0f, 0.0f}, -90.0f, 0.0f);

    // Initialize Metal
    InitMetal();

    running = true;
}

void Engine::resizeFrameBuffer()
{
    CA::MetalLayer *metalLayer = (CA::MetalLayer *)SDL_Metal_GetLayer(metalView);
    metalLayer->setDrawableSize(metalLayer->drawableSize()); // Ensure this is updated properly

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

    // Recreate the textures with the new size
    createDepthAndMSAATextures();

    // Get the new drawable and update the render pass descriptor
    metalDrawable = metalLayer->nextDrawable();
    updateRenderPassDescriptor();
}

// Destructor
Engine::~Engine()
{
    // Release resources
    if (metalDefaultLibrary)
        metalDefaultLibrary->release();

    // if (metalCommandQueue)
    //     metalCommandQueue->release();

    // if (metalCommandBuffer)
    //     metalCommandBuffer->release();

    // if (metalRenderPSO)
    //     metalRenderPSO->release();

    // if (msaaRenderTargetTexture)
    //     msaaRenderTargetTexture->release();

    // if (depthTexture)
    //     depthTexture->release();

    // if (renderPassDescriptor)
    //     renderPassDescriptor->release();

    if (device)
        device->release();

    // Destroy Metal view and SDL window
    if (metalView)
        SDL_Metal_DestroyView(metalView);
    if (window)
        SDL_DestroyWindow(window);

    SDL_Quit();
}

void Engine::InitMetal()
{
    // Get the CAMetalLayer from the SDL Metal view
    CA::MetalLayer *metalLayer = (CA::MetalLayer *)SDL_Metal_GetLayer(metalView);

    // Create Metal device
    device = MTL::CreateSystemDefaultDevice();

    metalLayer->setDevice(device);
    metalLayer->setPixelFormat(MTL::PixelFormatBGRA8Unorm);

    renderables.push_back(new Renderable(device, "/Users/chris/Documents/Programming/Repos/metal-render/bin/Release/assets/teapot.obj", glm::vec3(0.0f, 0.0f, 0.0f)));
    renderables.push_back(new Renderable(device, "/Users/chris/Documents/Programming/Repos/metal-render/bin/Release/assets/teapot.obj", glm::vec3(10.0f, 0.0f, 0.0f)));
    renderables.push_back(new Renderable(device, "/Users/chris/Documents/Programming/Repos/metal-render/bin/Release/assets/cow.obj", glm::vec3(0.0f, 50.0f, 0.0f)));
    renderables.push_back(new Renderable(device, "/Users/chris/Documents/Programming/Repos/metal-render/bin/Release/assets/teddy.obj", glm::vec3(50.0f, 50.0f, 0.0f)));

    createDefaultLibrary();
    createCommandQueue();
    createRenderPipeline();
    createDepthAndMSAATextures();
    createRenderPassDescriptor();

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    ImGui_ImplMetal_Init(device);
    ImGui_ImplSDL2_InitForMetal(window);
}

void Engine::createRenderPassDescriptor()
{
    CA::MetalLayer *metalLayer = (CA::MetalLayer *)SDL_Metal_GetLayer(metalView);
    metalDrawable = metalLayer->nextDrawable();

    renderPassDescriptor = MTL::RenderPassDescriptor::alloc()->init();

    MTL::RenderPassColorAttachmentDescriptor *colorAttachment = renderPassDescriptor->colorAttachments()->object(0);
    MTL::RenderPassDepthAttachmentDescriptor *depthAttachment = renderPassDescriptor->depthAttachment();

    colorAttachment->setTexture(msaaRenderTargetTexture);
    colorAttachment->setResolveTexture(metalDrawable->texture());
    colorAttachment->setLoadAction(MTL::LoadActionClear);
    colorAttachment->setClearColor(MTL::ClearColor(41.0f / 255.0f, 42.0f / 255.0f, 48.0f / 255.0f, 1.0));
    colorAttachment->setStoreAction(MTL::StoreActionMultisampleResolve);

    depthAttachment->setTexture(depthTexture);
    depthAttachment->setLoadAction(MTL::LoadActionClear);
    depthAttachment->setStoreAction(MTL::StoreActionDontCare);
    depthAttachment->setClearDepth(1.0);
}

void Engine::updateRenderPassDescriptor()
{
    if (renderPassDescriptor)
    {
        renderPassDescriptor->colorAttachments()->object(0)->setTexture(msaaRenderTargetTexture);
        renderPassDescriptor->colorAttachments()->object(0)->setResolveTexture(metalDrawable->texture());
        renderPassDescriptor->depthAttachment()->setTexture(depthTexture);
    }
}

void Engine::createDefaultLibrary()
{
    metalDefaultLibrary = device->newDefaultLibrary();

    if (!metalDefaultLibrary)
    {
        std::cerr << "Failed to load library from shaders.metallib." << std::endl;
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
    CA::MetalLayer *metalLayer = (CA::MetalLayer *)SDL_Metal_GetLayer(metalView);

    MTL::TextureDescriptor *msaaTextureDescriptor = MTL::TextureDescriptor::alloc()->init();
    msaaTextureDescriptor->setTextureType(MTL::TextureType2DMultisample);
    msaaTextureDescriptor->setPixelFormat(MTL::PixelFormatBGRA8Unorm);
    msaaTextureDescriptor->setWidth(metalLayer->drawableSize().width);
    msaaTextureDescriptor->setHeight(metalLayer->drawableSize().height);
    msaaTextureDescriptor->setSampleCount(sampleCount);
    msaaTextureDescriptor->setUsage(MTL::TextureUsageRenderTarget);

    msaaRenderTargetTexture = device->newTexture(msaaTextureDescriptor);

    MTL::TextureDescriptor *depthTextureDescriptor = MTL::TextureDescriptor::alloc()->init();
    depthTextureDescriptor->setTextureType(MTL::TextureType2DMultisample);
    depthTextureDescriptor->setPixelFormat(MTL::PixelFormatDepth32Float);
    depthTextureDescriptor->setWidth(metalLayer->drawableSize().width);
    depthTextureDescriptor->setHeight(metalLayer->drawableSize().height);
    depthTextureDescriptor->setUsage(MTL::TextureUsageRenderTarget);
    depthTextureDescriptor->setSampleCount(sampleCount);

    depthTexture = device->newTexture(depthTextureDescriptor);

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

    // Start ImGui frame
    ImGui_ImplMetal_NewFrame(renderPassDescriptor);
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    ImGuiIO &io = ImGui::GetIO();
    float dpiScaleFactor = static_cast<float>(metalDrawable->texture()->width()) / io.DisplaySize.x;

    // Show demo window (optional)
    static bool showDemoWindow = false;
    if (showDemoWindow)
        ImGui::ShowDemoWindow(&showDemoWindow);

    // Renderables window
    ImGui::Begin("Renderables");

    int index = 0;
    ImDrawList *drawList = ImGui::GetBackgroundDrawList();

    for (auto &renderable : renderables)
    {
        // Get renderable's name or default to "Renderable N"
        std::string renderableName = "Renderable " + std::to_string(index);

        // Create a collapsible header for each renderable
        if (ImGui::CollapsingHeader(renderableName.c_str()))
        {
            glm::vec4 viewport(0, 0, metalDrawable->texture()->width(),
                               metalDrawable->texture()->height());

            // Compute screen position
            glm::vec2 screenPos = camera.WorldToScreen(
                renderable->getPosition(),
                camera.GetProjectionMatrix(aspectRatio),
                camera.GetViewMatrix(),
                viewport);

            bool isInFrontOfCamera =
                (screenPos.x != -FLT_MAX && screenPos.y != -FLT_MAX);

            // Apply DPI scaling
            screenPos /= dpiScaleFactor;

            // Draw line and label if in front of the camera
            if (isInFrontOfCamera)
            {
                drawList->AddLine(ImVec2(io.DisplaySize.x / 2, io.DisplaySize.y / 2),
                                  ImVec2(screenPos.x, screenPos.y),
                                  IM_COL32(255, 255, 255, 255), 2.0f);

                drawList->AddText(ImVec2(screenPos.x, screenPos.y),
                                  IM_COL32(255, 255, 255, 255),
                                  renderableName.c_str());
            }

            // Display renderable info
            ImGui::Text("Position: (%.2f, %.2f, %.2f)",
                        renderable->getPosition().x,
                        renderable->getPosition().y,
                        renderable->getPosition().z);
            ImGui::Text("Status: %s",
                        isInFrontOfCamera ? "In front of the camera"
                                          : "Behind the camera or invalid");

            // Teleport and Look At buttons with unique IDs
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

    // Camera controls window
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

    // Teleport input fields
    static float teleportCoords[3] = {490.0f, -281.0f, -4387.0f};
    ImGui::InputFloat3("Teleport Coordinates", teleportCoords);

    if (ImGui::Button("Teleport##Camera"))
    {
        camera.Teleport(glm::vec3(teleportCoords[0], teleportCoords[1], teleportCoords[2]));
    }

    ImGui::End();

    // Render ImGui draw data
    ImGui::Render();
    ImDrawData *drawData = ImGui::GetDrawData();
    ImGui_ImplMetal_RenderDrawData(drawData, metalCommandBuffer, imguiRenderCommandEncoder);

    // End encoding
    imguiRenderCommandEncoder->endEncoding();
    imguiRenderCommandEncoder->release();
}

simd::float3 glmToSimd(const glm::vec3 &v)
{
    return simd::float3{v.x, v.y, v.z};
}

void Engine::sendRenderCommand()
{
    // Create a new command buffer
    metalCommandBuffer = metalCommandQueue->commandBuffer();

    // Update the render pass descriptor if necessary
    updateRenderPassDescriptor();

    // Create a new render pass descriptor
    MTL::RenderPassDescriptor *renderPassDescriptor = MTL::RenderPassDescriptor::alloc()->init();
    MTL::RenderPassColorAttachmentDescriptor *cd = renderPassDescriptor->colorAttachments()->object(0);
    cd->setTexture(metalDrawable->texture());
    cd->setLoadAction(MTL::LoadActionClear);
    cd->setClearColor(MTL::ClearColor(41.0f / 255.0f, 42.0f / 255.0f, 48.0f / 255.0f, 1.0f));
    cd->setStoreAction(MTL::StoreActionStore);

    // Add depth attachment
    MTL::TextureDescriptor *depthTextureDescriptor = MTL::TextureDescriptor::texture2DDescriptor(
        MTL::PixelFormatDepth32Float,
        metalDrawable->texture()->width(),
        metalDrawable->texture()->height(),
        false);
    depthTextureDescriptor->setUsage(MTL::TextureUsageRenderTarget);
    depthTextureDescriptor->setStorageMode(MTL::StorageModePrivate);

    MTL::Texture *depthTexture = device->newTexture(depthTextureDescriptor);
    renderPassDescriptor->depthAttachment()->setTexture(depthTexture);
    renderPassDescriptor->depthAttachment()->setClearDepth(1.0);
    renderPassDescriptor->depthAttachment()->setLoadAction(MTL::LoadActionClear);
    renderPassDescriptor->depthAttachment()->setStoreAction(MTL::StoreActionStore);

    // Start the render command encoder for 3D rendering
    MTL::RenderCommandEncoder *renderCommandEncoder = metalCommandBuffer->renderCommandEncoder(renderPassDescriptor);

    // Set the viewport
    renderCommandEncoder->setViewport(MTL::Viewport{0.0, 0.0,
                                                    static_cast<double>(metalDrawable->texture()->width()),
                                                    static_cast<double>(metalDrawable->texture()->height()),
                                                    0.0, 1.0});

    // Set the depth stencil state
    renderCommandEncoder->setDepthStencilState(depthStencilState);

    LightData lightData;
    lightData.ambientColor = glmToSimd(glm::vec3(0.2f, 0.2f, 0.2f)); // Ambient light color
    // project the sun downwards at a 45 degree angle
    lightData.lightDirection = glmToSimd(glm::vec3(1.0f, 1.0f, 1.0f));
    lightData.lightColor = glmToSimd(glm::vec3(1.0f, 1.0f, 1.0f)); // Light color

    // Create a buffer for the light data
    MTL::Buffer *lightBuffer = device->newBuffer(&lightData, sizeof(LightData), MTL::ResourceStorageModeShared);

    renderCommandEncoder->setFragmentBuffer(lightBuffer, 0, 1); // Bind the light buffer to the fragment shader

    // Encode your render commands for 3D models
    drawRenderables(renderCommandEncoder);

    renderCommandEncoder->endEncoding();

    // Start a new render command encoder for ImGui
    cd->setLoadAction(MTL::LoadActionLoad); // Change to Load to preserve 3D content

    drawImGui(renderPassDescriptor);

    // Present the drawable to the screen
    metalCommandBuffer->presentDrawable(metalDrawable);

    // Commit the command buffer and wait for completion
    metalCommandBuffer->commit();
    metalCommandBuffer->waitUntilCompleted();

    // Release resources
    renderPassDescriptor->release();
    metalCommandBuffer->release();
    metalDrawable = nullptr;
    renderCommandEncoder->release();
    depthTexture->release();
    depthTextureDescriptor->release();
}

void Engine::drawRenderables(MTL::RenderCommandEncoder *renderCommandEncoder)
{
    CA::MetalLayer *metalLayer = (CA::MetalLayer *)SDL_Metal_GetLayer(metalView);

    for (auto renderable : renderables)
    {
        renderable->draw(metalLayer, camera, renderCommandEncoder, metalRenderPSO, depthStencilState);
    }
}

void Engine::Run()
{
    Uint64 NOW = SDL_GetPerformanceCounter();
    Uint64 LAST = 0;
    double deltaTime = 0;

    const double fixedTimeStep = 1.0 / 60.0; // 60 updates per second
    double accumulator = 0.0;

    int windowWidth, windowHeight;
    SDL_GetWindowSize(window, &windowWidth, &windowHeight);

    while (running)
    {
        LAST = NOW;
        NOW = SDL_GetPerformanceCounter();
        deltaTime = (double)((NOW - LAST) / (double)SDL_GetPerformanceFrequency());
        accumulator += deltaTime;

        // Event handling
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL2_ProcessEvent(&event);

            if (event.type == SDL_QUIT)
                running = false;

            // Handle mouse button down/up for camera control
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

        // Mouse motion for camera look (click-and-drag)
        if (!ImGui::GetIO().WantCaptureMouse)
        {
            int mouseX, mouseY;
            SDL_GetMouseState(&mouseX, &mouseY);
            camera.OnMouseMove(mouseX, mouseY);
        }

        // Camera movement (smooth and frame-rate independent)
        if (!ImGui::GetIO().WantCaptureKeyboard)
        {
            const Uint8 *state = SDL_GetKeyboardState(NULL);
            camera.ProcessKeyboardInput(state, (float)deltaTime);
        }

        // Fixed update step
        while (accumulator >= fixedTimeStep)
        {
            // UpdateGameLogic(fixedTimeStep);
            accumulator -= fixedTimeStep;
        }

        // Rendering
        CA::MetalLayer *metalLayer = (CA::MetalLayer *)SDL_Metal_GetLayer(metalView);
        metalDrawable = metalLayer->nextDrawable();
        if (!metalDrawable)
        {
            std::cerr << "Failed to get next drawable, skipping frame." << std::endl;
            SDL_Delay(1); // Small delay to prevent busy looping
            continue;
        }

        // Your 3D rendering code here
        draw();
    }
}
