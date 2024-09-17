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

vertex VertexOut geometry_VertexShader(uint vertexID [[vertex_id]],
             constant VertexData* vertexData,
             constant TransformationData* transformationData) {
    VertexOut out;
    out.position = transformationData->perspectiveMatrix * transformationData->viewMatrix * transformationData->modelMatrix * vertexData[vertexID].position;
    out.color = vertexData[vertexID].color;

    // Transform the normal by the model matrix (ignore translation)
    out.normal = normalize((float3)(transformationData->modelMatrix * float4(vertexData[vertexID].normal, 0.0f)).xyz);
    return out;
}

fragment float4 geometry_FragmentShader(VertexOut in [[stage_in]], constant LightData &lightData [[buffer(1)]]) {
    // Ambient light
    float3 ambient = lightData.ambientColor;

    // Default diffuse lighting (set low intensity since normals aren't reliable)
    float3 lightDir = normalize(lightData.lightDirection);
    float diffuseIntensity = max(dot(in.normal, lightDir), 0.0) * 0.5; // Reduce intensity
    float3 diffuse = lightData.lightColor * diffuseIntensity;

    // Combine ambient and diffuse lighting
    float3 finalColor = ambient + diffuse;

    // Return the final color, modulated with the vertex color
    return float4(finalColor * in.color.rgb, in.color.a);
}
