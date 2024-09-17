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
#include <memory>
#include <vector>
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
    bool running = false;
    Camera camera;

    std::unique_ptr<SDL_Window, void(*)(SDL_Window*)> window;
    SDL_MetalView metalView = nullptr;

    MTL::Device* device = nullptr;
    CA::MetalDrawable* metalDrawable = nullptr;

    MTL::Library* metalDefaultLibrary = nullptr;
    MTL::CommandQueue* metalCommandQueue = nullptr;
    std::unique_ptr<MTL::CommandBuffer, void(*)(MTL::CommandBuffer*)> metalCommandBuffer;
    MTL::RenderPipelineState* metalRenderPSO = nullptr;

    std::vector<std::unique_ptr<Renderable>> renderables;

    MTL::DepthStencilState* depthStencilState = nullptr;
    std::unique_ptr<MTL::Texture, void(*)(MTL::Texture*)> msaaRenderTargetTexture;
    std::unique_ptr<MTL::Texture, void(*)(MTL::Texture*)> depthTexture;

    std::unique_ptr<MTL::RenderPassDescriptor, void(*)(MTL::RenderPassDescriptor*)> renderPassDescriptor;
    std::unique_ptr<MTL::Buffer, void(*)(MTL::Buffer*)> lightBuffer;

    int sampleCount = 4;

    void InitMetal();
    void createDefaultLibrary();
    void createCommandQueue();
    void createRenderPipeline();
    void resizeFrameBuffer();
    void createDepthAndMSAATextures();

    void drawRenderables(MTL::RenderCommandEncoder *renderEncoder);
    void drawImGui(MTL::RenderPassDescriptor *renderPassDescriptor);
    void sendRenderCommand();
    void draw();
};
