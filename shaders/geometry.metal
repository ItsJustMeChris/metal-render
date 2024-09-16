#include <metal_stdlib>
using namespace metal;

struct VertexData {
    float4 position;
    float4 color;
};

struct TransformationData
{
    float4x4 modelMatrix;
    float4x4 viewMatrix;
    float4x4 perspectiveMatrix;
};

struct VertexOut {
    float4 position [[position]];
    float4 color;
};

vertex VertexOut geometry_VertexShader(uint vertexID [[vertex_id]],
             constant VertexData* vertexData,
             constant TransformationData* transformationData) {
    VertexOut out;
    out.position = transformationData->perspectiveMatrix * transformationData->viewMatrix * transformationData->modelMatrix * vertexData[vertexID].position;
    out.color = vertexData[vertexID].color;
    return out;
}

fragment float4 geometry_FragmentShader(VertexOut in [[stage_in]]) {
    constexpr sampler textureSampler (mag_filter::linear,
                                      min_filter::linear);
    const float4 color = in.color;
    return color;
}
