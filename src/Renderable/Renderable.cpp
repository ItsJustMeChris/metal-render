#include "Renderable.hpp"
#include "Engine.hpp"

Renderable::Renderable(MTL::Device *device, Engine *engine, PipelineManager *pipelineManager, const std::string &pipelineName, std::shared_ptr<Model> model, const glm::vec3 &position, const simd::float4 &color)
    : device(device), engine(engine), model(model), position(position), color(color)
{
    // Get the pipeline state from the PipelineManager
    pipelineState = pipelineManager->getPipeline(pipelineName);
    if (!pipelineState)
    {
        std::cerr << "Failed to get pipeline state: " << pipelineName << std::endl;
    }

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
    glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), position);
    glm::mat4 viewMatrix = camera.GetViewMatrix();
    float aspectRatio = engine->getRenderer()->aspectRatio();
    glm::mat4 projectionMatrix = camera.GetProjectionMatrix(aspectRatio);

    // Update uniform buffer
    TransformationData transformationData = {modelMatrix, viewMatrix, projectionMatrix};
    memcpy(transformBuffer->contents(), &transformationData, sizeof(transformationData));

    // Set up render command encoder
    renderCommandEncoder->setFrontFacingWinding(MTL::WindingCounterClockwise);
    renderCommandEncoder->setRenderPipelineState(pipelineState);
    renderCommandEncoder->setDepthStencilState(depthStencilState);

    // Set vertex buffers
    renderCommandEncoder->setVertexBuffer(model->getVertexBuffer(), 0, 0);
    renderCommandEncoder->setVertexBuffer(transformBuffer, 0, 1);

    // Optionally set fragment buffer for color (if your shader requires it)
    // renderCommandEncoder->setFragmentBytes(&color, sizeof(color), 0);

    // Draw call
    renderCommandEncoder->drawPrimitives(MTL::PrimitiveTypeTriangle, (NS::UInteger)0, model->getVertexCount());
}
