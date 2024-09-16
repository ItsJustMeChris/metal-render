#pragma once

#include <Metal/Metal.hpp>
#include <QuartzCore/QuartzCore.hpp>
#include <vector>
#include <string>
#include <glm/glm.hpp>
#include <simd/simd.h>
#include <iostream>
#include "Camera.hpp"

struct VertexData
{
    simd::float4 position;
    simd::float4 color;
};

struct TransformationData
{
    glm::mat4 modelMatrix;
    glm::mat4 viewMatrix;
    glm::mat4 perspectiveMatrix;
};

class Renderable
{
public:
    Renderable(MTL::Device *device, const std::string &objFilePath, const glm::vec3 &position = glm::vec3(0.0f, 0.0f, 0.0f));
    ~Renderable();

    void draw(CA::MetalLayer *metalLayer, Camera &camera, MTL::RenderCommandEncoder *renderCommandEncoder, MTL::RenderPipelineState *metalRenderPSO, MTL::DepthStencilState *depthStencilState);
    glm::vec3 getPosition() const { return position; }

private:
    MTL::Device *device;
    MTL::Buffer *vertexBuffer;
    MTL::Buffer *transformBuffer;
    std::vector<VertexData> vertices;
    NS::UInteger vertexCount;

    glm::mat4 modelMatrix;
    glm::vec3 position;

    void loadOBJ(const std::string &filePath);
    void createBuffers();
};
