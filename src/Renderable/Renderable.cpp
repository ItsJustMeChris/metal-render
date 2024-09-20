#include "Renderable.hpp"
#include "Engine.hpp"

Renderable::Renderable(MTL::Device *device, Engine *engine, PipelineManager *pipelineManager, const std::string &pipelineName, std::shared_ptr<Model> model, const glm::vec3 &position, const std::string &name)
    : device(device), engine(engine), model(model), position(position)
{
    pipelineState = pipelineManager->getPipeline(pipelineName);
    if (!pipelineState)
    {
        std::cerr << "Failed to get pipeline state: " << pipelineName << std::endl;
    }

    modelMatrix = glm::translate(glm::mat4(1.0f), position);
    this->name = name;
    createBuffers();
}

Renderable::~Renderable()
{
    if (transformBuffer)
        transformBuffer->release();
}

void Renderable::createBuffers()
{
    transformBuffer = device->newBuffer(sizeof(TransformationData), MTL::ResourceStorageModeShared);
}

void Renderable::draw(Camera &camera, MTL::RenderCommandEncoder *renderCommandEncoder, MTL::DepthStencilState *depthStencilState)
{
    glm::mat4 viewMatrix = camera.GetViewMatrix();
    float aspectRatio = engine->getRenderer()->aspectRatio();
    glm::mat4 projectionMatrix = camera.GetProjectionMatrix(aspectRatio);

    TransformationData transformationData = {modelMatrix, viewMatrix, projectionMatrix};
    memcpy(transformBuffer->contents(), &transformationData, sizeof(transformationData));

    renderCommandEncoder->setFrontFacingWinding(MTL::WindingCounterClockwise);
    renderCommandEncoder->setRenderPipelineState(pipelineState);
    renderCommandEncoder->setDepthStencilState(depthStencilState);

    renderCommandEncoder->setVertexBuffer(transformBuffer, 0, 1);

    for (const auto &mesh : model->getMeshes())
    {
        mesh->draw(renderCommandEncoder);
    }
}

void Renderable::setPosition(const glm::vec3 &newPosition)
{
    position = newPosition;
    modelMatrix = glm::translate(glm::mat4(1.0f), position);
}

std::optional<glm::vec3> Renderable::intersect(const glm::vec3 &origin, const glm::vec3 &destination)
{
    glm::mat4 inverseModelMatrix = glm::inverse(modelMatrix);

    glm::vec4 localOrigin4 = inverseModelMatrix * glm::vec4(origin, 1.0f);
    glm::vec4 localDestination4 = inverseModelMatrix * glm::vec4(destination, 1.0f);

    glm::vec3 localOrigin = glm::vec3(localOrigin4);
    glm::vec3 localDestination = glm::vec3(localDestination4);

    if (auto intersection = model->intersect(localOrigin, localDestination))
    {
        glm::vec4 worldIntersection4 = modelMatrix * glm::vec4(intersection.value(), 1.0f);
        return glm::vec3(worldIntersection4);
    }
    else
    {
        return std::nullopt;
    }
}
