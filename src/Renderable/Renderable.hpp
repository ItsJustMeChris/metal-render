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

class Engine;

struct VertexData
{
    simd::float4 position;
    simd::float4 color;
    simd::float3 normal;
} __attribute__((aligned(16)));

struct TransformationData
{
    glm::mat4 modelMatrix;
    glm::mat4 viewMatrix;
    glm::mat4 perspectiveMatrix;
} __attribute__((aligned(16)));

struct LightData
{
    simd::float3 ambientColor;
    simd::float3 lightDirection;
    simd::float3 lightColor;
} __attribute__((aligned(16)));

class Renderable
{
public:
    Renderable(MTL::Device *device, Engine *engine, PipelineManager *pipelineManager, const std::string &pipelineName, const std::string &objFilePath, const glm::vec3 &position = glm::vec3(0.0f), const simd::float4 color = {0.5f, 0.5f, 0.5f, .3f});
    ~Renderable();

    void draw(Camera &camera, MTL::RenderCommandEncoder *renderCommandEncoder, MTL::DepthStencilState *depthStencilState);
    glm::vec3 getPosition() const { return position; }

    Engine *engine;

private:
    MTL::Device *device;
    MTL::Buffer *vertexBuffer;
    MTL::Buffer *transformBuffer;
    MTL::RenderPipelineState *pipelineState; // Store the pipeline state
    std::vector<VertexData> vertices;
    NS::UInteger vertexCount;

    simd::float4 color;
    glm::mat4 modelMatrix;
    glm::vec3 position;

    void loadOBJ(const std::string &filePath);
    void createBuffers();
    void calculateNormals();
};
