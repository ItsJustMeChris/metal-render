#pragma once

#include <Metal/Metal.hpp>
#include <simd/simd.h>
#include <string>
#include "tiny_obj_loader.h"
#include "Texture.hpp"

class Material
{
public:
    Material(MTL::Device *device, const tinyobj::material_t &mat_data, const std::string &baseDir);
    ~Material();

    simd::float3 ambient;
    simd::float3 diffuse;
    simd::float3 specular;
    float shininess;

    void bind(MTL::RenderCommandEncoder *encoder);

    MTL::Buffer *getMaterialBuffer() const { return materialBuffer; }
    MTL::Texture *getDiffuseTexture() const { return diffuseTexture; }

private:
    MTL::Device *device;
    MTL::Buffer *materialBuffer;
    MTL::Texture *diffuseTexture;
    std::shared_ptr<Texture> texture;

    void createBuffer();
    void loadTexture(const std::string &textureFilename, const std::string &baseDir);
};
