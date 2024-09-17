#include <metal_stdlib>
using namespace metal;

struct VertexData {
    float4 position;
    float4 color;
    float3 normal; // Add normal to the vertex data
};

struct TransformationData
{
    float4x4 modelMatrix;
    float4x4 viewMatrix;
    float4x4 perspectiveMatrix;
};

struct LightData {
    float3 ambientColor;
    float3 lightDirection; // Direction of the directional light ("sun")
    float3 lightColor;     // Color of the directional light
};

struct VertexOut {
    float4 position [[position]];
    float4 color;
    float3 normal; // Pass the normal to the fragment shader
};

vertex VertexOut debug_geometry_VertexShader(uint vertexID [[vertex_id]],
             constant VertexData* vertexData,
             constant TransformationData* transformationData) {
    VertexOut out;
    out.position = transformationData->perspectiveMatrix * transformationData->viewMatrix * transformationData->modelMatrix * vertexData[vertexID].position;
    out.color = vertexData[vertexID].color;

    // Transform the normal by the model matrix (ignore translation)
    out.normal = normalize((float3)(transformationData->modelMatrix * float4(vertexData[vertexID].normal, 0.0f)).xyz);
    return out;
}

fragment float4 debug_geometry_FragmentShader(VertexOut in [[stage_in]], constant LightData &lightData [[buffer(1)]]) {
    // debug the normal

    return float4(in.normal, 1.0);
}
