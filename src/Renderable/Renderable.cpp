#include "Renderable.hpp"
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

Renderable::Renderable(MTL::Device *device, const std::string &objFilePath, const glm::vec3 &position)
    : device(device), modelMatrix(glm::mat4(1.0f))
{
    loadOBJ(objFilePath);
    createBuffers();

    this->position = position;
}

Renderable::~Renderable()
{
    if (vertexBuffer)
        vertexBuffer->release();
    if (transformBuffer)
        transformBuffer->release();
}

void Renderable::loadOBJ(const std::string &filePath)
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

    //    helper lambda to generate a random color
    auto randomColor = []()
    {
        return static_cast<float>(rand()) / RAND_MAX;
    };

    // Process the loaded data and fill the vertices vector
    for (const auto &shape : shapes)
    {
        for (const auto &index : shape.mesh.indices)
        {
            VertexData vertex;
            vertex.position = {
                attrib.vertices[3 * index.vertex_index + 0],
                attrib.vertices[3 * index.vertex_index + 1],
                attrib.vertices[3 * index.vertex_index + 2],
                1.0f};

            vertex.color = {randomColor(), randomColor(), randomColor(), 1.0f};
            vertices.push_back(vertex);
        }
    }

    vertexCount = vertices.size();
    size_t bufferSize = vertexCount * sizeof(VertexData);

    vertexBuffer = device->newBuffer(vertices.data(), bufferSize, MTL::ResourceStorageModeShared);
}

void Renderable::createBuffers()
{
    transformBuffer = device->newBuffer(sizeof(TransformationData), MTL::ResourceStorageModeShared);
}

void Renderable::draw(CA::MetalLayer *metalLayer, Camera &camera, MTL::RenderCommandEncoder *renderCommandEncoder, MTL::RenderPipelineState *metalRenderPSO, MTL::DepthStencilState *depthStencilState)
{
    // Model matrix: simple translation
    glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), position);

    // View matrix: get from our Camera class
    glm::mat4 viewMatrix = camera.GetViewMatrix();

    // Projection matrix
    float aspectRatio = metalLayer->drawableSize().width / metalLayer->drawableSize().height;
    glm::mat4 projectionMatrix = glm::perspective(glm::radians(45.0f), aspectRatio, 0.1f, 100.0f);

    // Update uniform buffer
    TransformationData transformationData = {modelMatrix, viewMatrix, projectionMatrix};
    memcpy(transformBuffer->contents(), &transformationData, sizeof(transformationData));

    // Set up render command encoder
    renderCommandEncoder->setFrontFacingWinding(MTL::WindingCounterClockwise);
    renderCommandEncoder->setCullMode(MTL::CullModeBack);
    // renderCommandEncoder->setTriangleFillMode(MTL::TriangleFillModeLines);
    renderCommandEncoder->setRenderPipelineState(metalRenderPSO);
    renderCommandEncoder->setDepthStencilState(depthStencilState);

    // Set vertex buffers
    renderCommandEncoder->setVertexBuffer(vertexBuffer, 0, 0);
    renderCommandEncoder->setVertexBuffer(transformBuffer, 0, 1);

    // Draw call
    renderCommandEncoder->drawPrimitives(MTL::PrimitiveTypeTriangle, (NS::UInteger)0, vertexCount);
}
