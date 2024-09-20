#pragma once

#include <Metal/Metal.hpp>
#include <vector>
#include <memory>
#include "Material.hpp"
#include "VertexData.hpp"
#include <glm/glm.hpp>

class Mesh
{
public:
    Mesh(MTL::Device *device,
         const std::vector<VertexData> &vertices,
         const std::vector<uint32_t> &indices,
         std::shared_ptr<Material> material);
    ~Mesh();

    void draw(MTL::RenderCommandEncoder *encoder);

    // Methods to access vertex and index data
    const VertexData *getVertices() const;
    size_t getVertexCount() const;
    const uint32_t *getIndices() const;
    size_t getIndexCount() const;

private:
    MTL::Buffer *vertexBuffer;
    MTL::Buffer *indexBuffer;
    uint32_t indexCount;
    std::shared_ptr<Material> material;

    MTL::Device *device;
};
