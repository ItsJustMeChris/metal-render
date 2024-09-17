#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_metal.h>
#include <Metal/Metal.hpp>
#include <QuartzCore/QuartzCore.hpp>
#include <memory>
#include <vector>
#include "Renderable.hpp"
#include "Camera.hpp"
#include "PipelineManager.hpp"

class Engine;

class Renderer
{
public:
    Renderer(SDL_MetalView metalView, Engine* engine);
    ~Renderer();

    void render(Camera& camera, class ImGuiHandler& imguiHandler);
    MTL::Device* getDevice() const { return device; }

    std::vector<std::unique_ptr<Renderable>>& getRenderables() { return renderables; }
    float aspectRatio() const { return static_cast<float>(metalDrawable->texture()->width()) / static_cast<float>(metalDrawable->texture()->height()); }
    glm::vec4 viewport() const { return glm::vec4(0, 0, metalDrawable->texture()->width(), metalDrawable->texture()->height()); }
    glm::vec2 dimensions() const { return glm::vec2(metalDrawable->texture()->width(), metalDrawable->texture()->height()); }

    Engine* engine;

private:
    void initMetal();
    void createDepthAndMSAATextures();
    void createRenderPipelines();
    void resizeDrawable();

    SDL_MetalView metalView = nullptr;
    MTL::Device* device = nullptr;
    CA::MetalDrawable* metalDrawable = nullptr;
    MTL::CommandQueue* metalCommandQueue = nullptr;
    MTL::DepthStencilState* depthStencilState = nullptr;

    std::unique_ptr<MTL::CommandBuffer, void(*)(MTL::CommandBuffer*)> metalCommandBuffer;
    std::unique_ptr<MTL::Texture, void(*)(MTL::Texture*)> msaaRenderTargetTexture;
    std::unique_ptr<MTL::Texture, void(*)(MTL::Texture*)> depthTexture;

    std::unique_ptr<MTL::RenderPassDescriptor, void(*)(MTL::RenderPassDescriptor*)> renderPassDescriptor;
    std::unique_ptr<MTL::Buffer, void(*)(MTL::Buffer*)> lightBuffer;

    int sampleCount = 4;
    std::vector<std::unique_ptr<Renderable>> renderables;

    // Add a pointer to the PipelineManager
    PipelineManager* pipelineManager;

    void drawRenderables(MTL::RenderCommandEncoder* renderEncoder, Camera& camera);
    void setupEventHandlers();
};
