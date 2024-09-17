#pragma once

#include <Metal/Metal.hpp>
#include <vector>
#include <string>
#include <simd/simd.h>
#include <iostream>
#include "tiny_obj_loader.h"

// Vertex data structure
struct VertexData
{
    simd::float4 position;
    simd::float4 color;
    simd::float3 normal;
} __attribute__((aligned(16)));

class Model
{
public:
    Model(MTL::Device* device, const std::string& objFilePath, const simd::float4& color = {0.5f, 0.5f, 0.5f, 1.0f});
    ~Model();

    MTL::Buffer* getVertexBuffer() const { return vertexBuffer; }
    NS::UInteger getVertexCount() const { return vertexCount; }

private:
    MTL::Device* device;
    MTL::Buffer* vertexBuffer;
    std::vector<VertexData> vertices;
    NS::UInteger vertexCount;

    void loadOBJ(const std::string& filePath);
    void calculateNormals();
    void createBuffers();
    simd::float4 color;
};
