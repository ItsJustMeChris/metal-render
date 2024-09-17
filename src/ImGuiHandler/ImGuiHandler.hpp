#pragma once

#include <SDL2/SDL.h>
#include <Metal/Metal.hpp>
#include <imgui.h>
#include <backends/imgui_impl_sdl2.h>
#include <backends/imgui_impl_metal.h>

class Engine;

class ImGuiHandler
{
public:
    ImGuiHandler(SDL_Window *window, MTL::Device *device);
    ~ImGuiHandler();

    void processEvent(SDL_Event *event);

    void render(MTL::CommandBuffer *commandBuffer, MTL::RenderPassDescriptor *renderPassDescriptor, uint32_t screenWidth, uint32_t screenHeight);

    bool wantsMouseCapture() const;
    bool wantsKeyboardCapture() const;

    Engine *engine;

private:
    void drawInterface();

    SDL_Window *window;
};
