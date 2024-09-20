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
