#pragma once
#include <Metal/Metal.hpp>
#include <unordered_map>
#include <string>

class PipelineManager
{
public:
    PipelineManager(MTL::Device *device);
    ~PipelineManager();

    MTL::RenderPipelineState *getPipeline(const std::string &name);

    void createPipeline(const std::string &name, const MTL::RenderPipelineDescriptor *descriptor);

    MTL::Library *library;

private:
    MTL::Device *device;
    std::unordered_map<std::string, MTL::RenderPipelineState *> pipelines;
};
