#include "Model.hpp"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

Model::Model(MTL::Device* device, const std::string& objFilePath, const simd::float4& color)
    : device(device), color(color)
{
    loadOBJ(objFilePath);
    createBuffers();
}

Model::~Model()
{
    if (vertexBuffer)
        vertexBuffer->release();
}

void Model::loadOBJ(const std::string& filePath)
{
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filePath.c_str());

    if (!warn.empty())
    {
        std::cout << "TinyObjLoader warning: " << warn << std::endl;
    }

    if (!err.empty())
    {
        std::cerr << "TinyObjLoader error: " << err << std::endl;
    }

    if (!ret)
    {
        throw std::runtime_error("Failed to load OBJ file: " + filePath);
    }

    // Process the loaded data and fill the vertices vector
    for (const auto& shape : shapes)
    {
        for (const auto& index : shape.mesh.indices)
        {
            VertexData vertex;
            vertex.position = {
                attrib.vertices[3 * index.vertex_index + 0],
                attrib.vertices[3 * index.vertex_index + 1],
                attrib.vertices[3 * index.vertex_index + 2],
                1.0f};

            if (index.normal_index >= 0 && !attrib.normals.empty())
            {
                vertex.normal = {
                    attrib.normals[3 * index.normal_index + 0],
                    attrib.normals[3 * index.normal_index + 1],
                    attrib.normals[3 * index.normal_index + 2]};
            }
            else
            {
                // If normals are not provided, we'll calculate them later
                vertex.normal = {0.0f, 0.0f, 0.0f};
            }

            vertex.color = color;
            vertices.push_back(vertex);
        }
    }

    // If normals were not provided, calculate them
    if (attrib.normals.empty())
    {
        calculateNormals();
    }

    vertexCount = vertices.size();
}

void Model::calculateNormals()
{
    std::vector<simd::float3> faceNormals(vertices.size() / 3);
    std::vector<std::vector<int>> vertexFaces(vertices.size());

    // Calculate face normals
    for (size_t i = 0; i < vertices.size(); i += 3)
    {
        simd::float3 v0 = simd::float3{vertices[i].position[0], vertices[i].position[1], vertices[i].position[2]};
        simd::float3 v1 = simd::float3{vertices[i + 1].position[0], vertices[i + 1].position[1], vertices[i + 1].position[2]};
        simd::float3 v2 = simd::float3{vertices[i + 2].position[0], vertices[i + 2].position[1], vertices[i + 2].position[2]};

        simd::float3 edge1 = v1 - v0;
        simd::float3 edge2 = v2 - v0;
        simd::float3 normal = simd::normalize(simd::cross(edge1, edge2));

        faceNormals[i / 3] = normal;

        vertexFaces[i].push_back(static_cast<int>(i / 3));
        vertexFaces[i + 1].push_back(static_cast<int>(i / 3));
        vertexFaces[i + 2].push_back(static_cast<int>(i / 3));
    }

    // Average face normals to get vertex normals
    for (size_t i = 0; i < vertices.size(); ++i)
    {
        simd::float3 normal = {0.0f, 0.0f, 0.0f};
        for (int faceIndex : vertexFaces[i])
        {
            normal += faceNormals[faceIndex];
        }
        normal = simd::normalize(normal);
        vertices[i].normal = normal;
    }
}

void Model::createBuffers()
{
    size_t bufferSize = vertexCount * sizeof(VertexData);
    vertexBuffer = device->newBuffer(vertices.data(), bufferSize, MTL::ResourceStorageModeShared);
}
