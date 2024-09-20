#pragma once

#include <Metal/Metal.hpp>
#include <vector>
#include <string>
#include <simd/simd.h>
#include <iostream>
#include "tiny_obj_loader.h"
#include <unordered_map>
#include <memory>
#include "Mesh.hpp"
#include <glm/glm.hpp>

class Model
{
public:
    Model(MTL::Device *device, const std::string &objFilePath);
    ~Model();

    const std::vector<std::shared_ptr<Mesh>> &getMeshes() const { return meshes; }

    std::optional<glm::vec3> intersect(const glm::vec3 &origin, const glm::vec3 &destination);

private:
    MTL::Device *device;
    std::vector<std::shared_ptr<Mesh>> meshes;

    void loadOBJ(const std::string &filePath);
    void calculateNormals(std::vector<VertexData> &vertices, const std::vector<uint32_t> &indices);

    std::unordered_map<std::string, std::shared_ptr<Material>> materials;

    std::string getBaseDir(const std::string &filepath);

private:
    bool rayIntersectsTriangle(const glm::vec3 &orig, const glm::vec3 &dir,
                               const glm::vec3 &v0, const glm::vec3 &v1, const glm::vec3 &v2,
                               float &t);
};
