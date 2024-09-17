#include "PipelineManager.hpp"
#include "Engine.hpp"
#include <iostream>

PipelineManager::PipelineManager(MTL::Device* device) : device(device), library(nullptr) {
    library = device->newDefaultLibrary();
    if (!library) {
        std::cerr << "Failed to load default library." << std::endl;
    }

    printf("Loaded default library\n");
}

PipelineManager::~PipelineManager() {
    for (auto& pair : pipelines) {
        if (pair.second) {
            pair.second->release();
        }
    }
    if (library) {
        library->release();
    }
}

void PipelineManager::createPipeline(const std::string& name, const MTL::RenderPipelineDescriptor* descriptor) {
    NS::Error* error = nullptr;
    MTL::RenderPipelineState* pipelineState = device->newRenderPipelineState(descriptor, &error);
    if (!pipelineState) {
        std::cerr << "Failed to create pipeline state '" << name << "': " << error->localizedDescription()->utf8String() << std::endl;
    } else {
        pipelines[name] = pipelineState;
    }
}

MTL::RenderPipelineState* PipelineManager::getPipeline(const std::string& name) {
    auto it = pipelines.find(name);
    if (it != pipelines.end()) {
        return it->second;
    }
    return nullptr;
}
