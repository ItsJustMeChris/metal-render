#pragma once

#include <Metal/Metal.hpp>
#include <QuartzCore/QuartzCore.hpp>
#include <vector>
#include <string>
#include <glm/glm.hpp>
#include <simd/simd.h>
#include <iostream>
#include "Camera.hpp"
#include "PipelineManager.hpp"
#include "Model.hpp"
#include <memory>

class Engine;

// Transformation data structure
struct TransformationData
{
    glm::mat4 modelMatrix;
    glm::mat4 viewMatrix;
    glm::mat4 perspectiveMatrix;
} __attribute__((aligned(16)));

class Renderable
{
public:
    Renderable(MTL::Device* device, Engine* engine, PipelineManager* pipelineManager, const std::string& pipelineName, std::shared_ptr<Model> model, const glm::vec3& position = glm::vec3(0.0f), const simd::float4& color = {0.5f, 0.5f, 0.5f, 1.0f});
    ~Renderable();

    void draw(Camera& camera, MTL::RenderCommandEncoder* renderCommandEncoder, MTL::DepthStencilState* depthStencilState);
    glm::vec3 getPosition() const { return position; }

    Engine* engine;

private:
    MTL::Device* device;
    MTL::Buffer* transformBuffer;
    MTL::RenderPipelineState* pipelineState; // Store the pipeline state

    std::shared_ptr<Model> model; // Shared pointer to the Model
    glm::mat4 modelMatrix;
    glm::vec3 position;

    simd::float4 color; // Instance-specific color

    void createBuffers();
};
