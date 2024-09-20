#include "Material.hpp"
#include "Texture.hpp"
#include <iostream>
#include <fstream>
#include <filesystem>

Material::Material(MTL::Device *device, const tinyobj::material_t &mat_data, const std::string &baseDir)
    : device(device), diffuseTexture(nullptr), materialBuffer(nullptr)
{
    ambient = simd::float3{mat_data.ambient[0], mat_data.ambient[1], mat_data.ambient[2]};

    if (ambient[0] == 0.0f && ambient[1] == 0.0f && ambient[2] == 0.0f)
    {
        ambient = simd::float3{0.2f, 0.2f, 0.2f};
    }

    diffuse = simd::float3{mat_data.diffuse[0], mat_data.diffuse[1], mat_data.diffuse[2]};
    specular = simd::float3{mat_data.specular[0], mat_data.specular[1], mat_data.specular[2]};
    shininess = mat_data.shininess;

    createBuffer();

    if (!mat_data.diffuse_texname.empty())
    {
        loadTexture(mat_data.diffuse_texname, baseDir);
    }
}

Material::~Material()
{
    if (diffuseTexture)
        diffuseTexture->release();
    if (materialBuffer)
        materialBuffer->release();
}

void Material::createBuffer()
{
    struct MaterialData
    {
        simd::float3 ambient;
        simd::float3 diffuse;
        simd::float3 specular;
        float shininess;
    } __attribute__((aligned(16)));

    MaterialData matData;
    matData.ambient = ambient;
    matData.diffuse = diffuse;
    matData.specular = specular;
    matData.shininess = shininess;

    materialBuffer = device->newBuffer(&matData, sizeof(MaterialData), MTL::ResourceStorageModeShared);
}

void Material::bind(MTL::RenderCommandEncoder *encoder)
{
    encoder->setFragmentBuffer(materialBuffer, 0, 2);

    if (diffuseTexture)
    {
        encoder->setFragmentTexture(diffuseTexture, 0);
    }
}

void Material::loadTexture(const std::string &textureFilename, const std::string &baseDir)
{
    std::string texturePath = baseDir + textureFilename;
    if (std::filesystem::exists(texturePath))
    {
        texture = std::make_shared<Texture>(texturePath.c_str(), device);
        if (texture->getMTLTexture())
        {
            diffuseTexture = texture->getMTLTexture();
            diffuseTexture->retain();
        }
    }
    else
    {
        std::cerr << "Texture file not found: " << texturePath << std::endl;
    }
}
