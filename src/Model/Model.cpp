#include "Model.hpp"
#include <filesystem>
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

Model::Model(MTL::Device *device, const std::string &objFilePath)
    : device(device)
{
    loadOBJ(objFilePath);
}

Model::~Model()
{
}

std::string Model::getBaseDir(const std::string &filepath)
{
    std::filesystem::path path = filepath;
    return path.parent_path().string() + "/";
}

void Model::loadOBJ(const std::string &filePath)
{
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materialsData;
    std::string warn, err;
    std::string baseDir = getBaseDir(filePath);

    bool ret = tinyobj::LoadObj(&attrib, &shapes, &materialsData, &warn, &err, filePath.c_str(), baseDir.c_str(), true);

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

    for (const auto &mat_data : materialsData)
    {
        auto material = std::make_shared<Material>(device, mat_data, baseDir);
        materials[mat_data.name] = material;
    }

    std::unordered_map<int, std::vector<VertexData>> perMaterialVertices;
    std::unordered_map<int, std::vector<uint32_t>> perMaterialIndices;
    std::unordered_map<int, uint32_t> perMaterialIndexOffset;
    std::unordered_map<int, std::unordered_map<VertexData, uint32_t>> perMaterialVertexToIndexMap;

    for (const auto &shape : shapes)
    {
        size_t index_offset = 0;
        for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++)
        {
            int fv = shape.mesh.num_face_vertices[f];

            int material_id = shape.mesh.material_ids[f];

            if (material_id < 0)
                material_id = -1;

            for (size_t v = 0; v < fv; v++)
            {
                tinyobj::index_t idx = shape.mesh.indices[index_offset + v];

                VertexData vertex;

                vertex.position = {
                    attrib.vertices[3 * idx.vertex_index + 0],
                    attrib.vertices[3 * idx.vertex_index + 1],
                    attrib.vertices[3 * idx.vertex_index + 2],
                    1.0f};

                if (idx.normal_index >= 0)
                {
                    vertex.normal = {
                        attrib.normals[3 * idx.normal_index + 0],
                        attrib.normals[3 * idx.normal_index + 1],
                        attrib.normals[3 * idx.normal_index + 2],
                    };
                }
                else
                {
                    vertex.normal = {0.0f, 0.0f, 0.0f};
                }

                if (idx.texcoord_index >= 0)
                {
                    vertex.texcoord = {
                        attrib.texcoords[2 * idx.texcoord_index + 0],
                        attrib.texcoords[2 * idx.texcoord_index + 1]};
                }
                else
                {
                    vertex.texcoord = {0.0f, 0.0f};
                }

                auto &vertices = perMaterialVertices[material_id];
                auto &indices = perMaterialIndices[material_id];
                auto &vertexToIndexMap = perMaterialVertexToIndexMap[material_id];

                auto it = vertexToIndexMap.find(vertex);
                if (it != vertexToIndexMap.end())
                {
                    indices.push_back(it->second);
                }
                else
                {
                    uint32_t index = static_cast<uint32_t>(vertices.size());
                    vertices.push_back(vertex);
                    indices.push_back(index);
                    vertexToIndexMap[vertex] = index;
                }
            }
            index_offset += fv;
        }
    }

    for (const auto &[material_id, vertices] : perMaterialVertices)
    {
        const auto &indices = perMaterialIndices[material_id];
        std::vector<VertexData> meshVertices = vertices;

        if (attrib.normals.empty())
        {
            calculateNormals(meshVertices, indices);
        }

        std::shared_ptr<Material> material;
        if (material_id >= 0 && material_id < static_cast<int>(materialsData.size()))
        {
            const auto &mat_data = materialsData[material_id];
            material = materials[mat_data.name];
        }
        else
        {
            if (materials.find("default") == materials.end())
            {
                tinyobj::material_t defaultMatData;
                defaultMatData.name = "default";
                defaultMatData.diffuse[0] = 0.5f;
                defaultMatData.diffuse[1] = 0.5f;
                defaultMatData.diffuse[2] = 0.5f;

                material = std::make_shared<Material>(device, defaultMatData, baseDir);
                materials["default"] = material;
            }
            else
            {
                material = materials["default"];
            }
        }

        auto mesh = std::make_shared<Mesh>(device, meshVertices, indices, material);
        meshes.push_back(mesh);
    }
}

void Model::calculateNormals(std::vector<VertexData> &vertices, const std::vector<uint32_t> &indices)
{
    for (size_t i = 0; i < indices.size(); i += 3)
    {
        uint32_t idx0 = indices[i];
        uint32_t idx1 = indices[i + 1];
        uint32_t idx2 = indices[i + 2];

        simd::float3 v0 = simd::float3{vertices[idx0].position[0], vertices[idx0].position[1], vertices[idx0].position[2]};
        simd::float3 v1 = simd::float3{vertices[idx1].position[0], vertices[idx1].position[1], vertices[idx1].position[2]};
        simd::float3 v2 = simd::float3{vertices[idx2].position[0], vertices[idx2].position[1], vertices[idx2].position[2]};

        simd::float3 edge1 = v1 - v0;
        simd::float3 edge2 = v2 - v0;
        simd::float3 normal = simd::normalize(simd::cross(edge1, edge2));

        vertices[idx0].normal += normal;
        vertices[idx1].normal += normal;
        vertices[idx2].normal += normal;
    }

    for (auto &vertex : vertices)
    {
        vertex.normal = simd::normalize(vertex.normal);
    }
}

bool Model::rayIntersectsTriangle(const glm::vec3 &orig, const glm::vec3 &dir,
                                  const glm::vec3 &v0, const glm::vec3 &v1, const glm::vec3 &v2,
                                  float &t)
{
    const float EPSILON = 1e-8f;
    glm::vec3 edge1 = v1 - v0;
    glm::vec3 edge2 = v2 - v0;
    glm::vec3 h = glm::cross(dir, edge2);
    float a = glm::dot(edge1, h);

    if (a > -EPSILON && a < EPSILON)
        return false; // Ray is parallel to triangle.

    float f = 1.0f / a;
    glm::vec3 s = orig - v0;
    float u = f * glm::dot(s, h);

    if (u < 0.0f || u > 1.0f)
        return false;

    glm::vec3 q = glm::cross(s, edge1);
    float v = f * glm::dot(dir, q);

    if (v < 0.0f || u + v > 1.0f)
        return false;

    t = f * glm::dot(edge2, q);

    if (t > EPSILON)
        return true;

    return false;
}

std::optional<glm::vec3> Model::Intersect(const glm::vec3 &origin, const glm::vec3 &destination)
{
    glm::vec3 direction = glm::normalize(destination - origin);
    float maxDistance = glm::length(destination - origin);

    float closestT = std::numeric_limits<float>::max();
    glm::vec3 closestIntersection;
    bool hasIntersection = false;

    for (const auto &mesh : meshes)
    {
        const VertexData *vertices = mesh->getVertices();
        size_t vertexCount = mesh->getVertexCount();
        const uint32_t *indices = mesh->getIndices();
        size_t indexCount = mesh->getIndexCount();

        // Loop over triangles
        for (size_t i = 0; i < indexCount; i += 3)
        {
            uint32_t idx0 = indices[i + 0];
            uint32_t idx1 = indices[i + 1];
            uint32_t idx2 = indices[i + 2];

            const simd::float4 &v0 = vertices[idx0].position;
            const simd::float4 &v1 = vertices[idx1].position;
            const simd::float4 &v2 = vertices[idx2].position;

            glm::vec3 p0(v0[0], v0[1], v0[2]);
            glm::vec3 p1(v1[0], v1[1], v1[2]);
            glm::vec3 p2(v2[0], v2[1], v2[2]);

            float t = 0.0f;

            if (rayIntersectsTriangle(origin, direction, p0, p1, p2, t))
            {
                if (t >= 0.0f && t <= maxDistance && t < closestT)
                {
                    closestT = t;
                    closestIntersection = origin + t * direction;
                    hasIntersection = true;
                }
            }
        }
    }

    if (hasIntersection)
    {
        return closestIntersection;
    }
    else
    {
        return std::nullopt;
    }
}
