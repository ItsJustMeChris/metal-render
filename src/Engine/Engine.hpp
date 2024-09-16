#pragma once

#include <string>
#include <SDL2/SDL.h>
#include <SDL2/SDL_metal.h>
#include <Foundation/Foundation.hpp>
#include <Metal/Metal.hpp>
#include <QuartzCore/QuartzCore.hpp>
#include <iostream>
#include <cstdio>
#include <simd/simd.h>
#include <filesystem>
#include "Texture.hpp"
#include "AAPLMathUtilities.h"
#include <glm/glm.hpp>
#include "Camera.hpp"
#include "Renderable.hpp"

class Engine
{
public:
    Engine(const std::string &title);
    ~Engine();

    void Run();

private:
    SDL_Window *window = nullptr;
    SDL_MetalView metalView = nullptr;
    bool running = false;

    Camera camera;

    MTL::Device *device = nullptr;
    CA::MetalDrawable *metalDrawable;

    MTL::Library *metalDefaultLibrary;
    MTL::CommandQueue *metalCommandQueue;
    MTL::CommandBuffer *metalCommandBuffer;
    MTL::RenderPipelineState *metalRenderPSO;

    std::vector<Renderable *> renderables;

    void createDepthAndMSAATextures();
    void createRenderPassDescriptor();
    void updateRenderPassDescriptor();

    MTL::DepthStencilState *depthStencilState;
    MTL::RenderPassDescriptor *renderPassDescriptor;
    MTL::Texture *msaaRenderTargetTexture = nullptr;
    MTL::Texture *depthTexture;
    int sampleCount = 4;

    void InitMetal();
    void createDefaultLibrary();
    void createCommandQueue();
    void createRenderPipeline();
    void resizeFrameBuffer();

    void drawRenderables(MTL::RenderCommandEncoder *renderEncoder);
    void drawImGui(MTL::RenderPassDescriptor *renderPassDescriptor);
    void sendRenderCommand();
    void draw();
};
