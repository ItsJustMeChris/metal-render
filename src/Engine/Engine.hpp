#pragma once

#include <string>
#include <memory>
#include <SDL2/SDL.h>
#include <SDL2/SDL_metal.h>
#include "Renderer.hpp"
#include "Camera.hpp"

class ImGuiHandler;

class Engine
{
public:
    Engine(const std::string &title);
    ~Engine();

    void Run();

    Renderer *getRenderer() { return renderer.get(); }
    ImGuiHandler *getImGuiHandler() { return imguiHandler.get(); }
    Camera *getCamera() { return &camera; }

private:
    void processEvents(float deltaTime);
    void update(float deltaTime);
    void draw();

    bool running = false;
    std::unique_ptr<SDL_Window, void (*)(SDL_Window *)> window;
    SDL_MetalView metalView = nullptr;

    std::unique_ptr<Renderer> renderer;
    std::unique_ptr<ImGuiHandler> imguiHandler;
    Camera camera;
};
