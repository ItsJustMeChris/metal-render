#include "Mesh.hpp"

Mesh::Mesh(MTL::Device *device,
           const std::vector<VertexData> &vertices,
           const std::vector<uint32_t> &indices,
           std::shared_ptr<Material> material)
    : device(device), material(material), indexCount(static_cast<uint32_t>(indices.size()))
{
    size_t vertexBufferSize = sizeof(VertexData) * vertices.size();
    vertexBuffer = device->newBuffer(vertices.data(), vertexBufferSize, MTL::ResourceStorageModeShared);

    size_t indexBufferSize = sizeof(uint32_t) * indices.size();
    indexBuffer = device->newBuffer(indices.data(), indexBufferSize, MTL::ResourceStorageModeShared);
}

Mesh::~Mesh()
{
    if (vertexBuffer)
        vertexBuffer->release();
    if (indexBuffer)
        indexBuffer->release();
}

void Mesh::draw(MTL::RenderCommandEncoder *encoder)
{
    encoder->setVertexBuffer(vertexBuffer, 0, 0);

    // Set transform buffer is already set by Renderable

    encoder->setFragmentBuffer(material->getMaterialBuffer(), 0, 2);

    if (material->getDiffuseTexture())
    {
        encoder->setFragmentTexture(material->getDiffuseTexture(), 0);
    }

    MTL::SamplerDescriptor *samplerDesc = MTL::SamplerDescriptor::alloc()->init();
    samplerDesc->setMinFilter(MTL::SamplerMinMagFilterLinear);
    samplerDesc->setMagFilter(MTL::SamplerMinMagFilterLinear);
    samplerDesc->setMipFilter(MTL::SamplerMipFilterLinear);
    MTL::SamplerState *sampler = device->newSamplerState(samplerDesc);
    encoder->setFragmentSamplerState(sampler, 0);
    samplerDesc->release();
    sampler->release();

    encoder->drawIndexedPrimitives(MTL::PrimitiveType::PrimitiveTypeTriangle, indexCount, MTL::IndexType::IndexTypeUInt32, indexBuffer, 0);
}
