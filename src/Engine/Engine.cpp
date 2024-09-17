#include "Engine.hpp"
#include "ImGuiHandler.hpp"

Engine::Engine(const std::string &title)
    : window(nullptr, SDL_DestroyWindow)
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

    // Initialize Renderer
    renderer = std::make_unique<Renderer>(metalView);

    // Initialize ImGui Handler
    imguiHandler = std::make_unique<ImGuiHandler>(window.get(), renderer->getDevice());

    // Set up camera
    camera = Camera({0.0f, 0.0f, 1.0f}, {0.0f, 1.0f, 0.0f}, -90.0f, 0.0f);

    running = true;

    imguiHandler->engine = this;
}

Engine::~Engine()
{
    if (metalView)
        SDL_Metal_DestroyView(metalView);

    SDL_Quit();
}

void Engine::processEvents(float deltaTime)
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        imguiHandler->processEvent(&event);

        if (event.type == SDL_QUIT)
            running = false;

        if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT)
        {
            if (!imguiHandler->wantsMouseCapture())
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

    if (!imguiHandler->wantsMouseCapture())
    {
        int mouseX, mouseY;
        SDL_GetMouseState(&mouseX, &mouseY);
        camera.OnMouseMove(mouseX, mouseY);
    }

    if (!imguiHandler->wantsKeyboardCapture())
    {
        const Uint8 *state = SDL_GetKeyboardState(NULL);
        camera.ProcessKeyboardInput(state, deltaTime);
    }
}

void Engine::update(float deltaTime)
{
    // Update logic here (if any)
}

void Engine::draw()
{
    renderer->render(camera, *imguiHandler);
}

void Engine::Run()
{
    Uint64 NOW = SDL_GetPerformanceCounter();
    Uint64 LAST = 0;
    double deltaTime = 0;

    // This effectively caps the framerate to 120 FPS
    const double fixedTimeStep = 1.0 / 120.0;
    double accumulator = 0.0;

    while (running)
    {
        LAST = NOW;
        NOW = SDL_GetPerformanceCounter();
        deltaTime = (double)((NOW - LAST) / (double)SDL_GetPerformanceFrequency());
        accumulator += deltaTime;

        processEvents(static_cast<float>(deltaTime));

        while (accumulator >= fixedTimeStep)
        {
            update(static_cast<float>(fixedTimeStep));
            accumulator -= fixedTimeStep;
        }

        draw();
    }
}
