#include <metal_stdlib>
using namespace metal;

struct VertexData {
    float4 position;
    float3 normal;
    float2 texcoord;
};

struct TransformationData {
    float4x4 modelMatrix;
    float4x4 viewMatrix;
    float4x4 projectionMatrix;
};

struct LightData {
    float3 ambientColor;
    float3 lightDirection;
    float3 lightColor;
};

struct MaterialData {
    float3 ambient;
    float3 diffuse;
    float3 specular;
    float shininess;
};

struct VertexOut {
    float4 position [[position]];
    float3 normal;
    float2 texcoord;
    float3 fragPos;
};

vertex VertexOut geometry_VertexShader(
    uint vertexID [[vertex_id]],
    constant VertexData* vertexData [[buffer(0)]],
    constant TransformationData& trans [[buffer(1)]]
) {
    VertexOut out;
    float4 position = vertexData[vertexID].position;
    float3 normal = vertexData[vertexID].normal;

    float4 worldPosition = trans.modelMatrix * position;
    out.position = trans.projectionMatrix * trans.viewMatrix * worldPosition;
    out.fragPos = worldPosition.xyz;
    out.normal = normalize((trans.modelMatrix * float4(normal, 0.0)).xyz);
    out.texcoord = vertexData[vertexID].texcoord;
    return out;
}

fragment float4 geometry_FragmentShader(
    VertexOut in [[stage_in]],
    constant LightData& lightData [[buffer(1)]],
    constant MaterialData& material [[buffer(2)]],
    texture2d<float> diffuseTexture [[texture(0)]],
    sampler textureSampler [[sampler(0)]]
) {
    // Normalize inputs
    float3 normal = normalize(in.normal);
    float3 lightDir = normalize(lightData.lightDirection);

    // Ambient component with reduced strength
    float ambientStrength = 0.1; // Reduce this to make dark areas more pronounced
    float3 ambient = lightData.ambientColor * material.ambient * ambientStrength;

    // Diffuse component
    float NdotL = max(dot(normal, lightDir), 0.0);
    float3 diffuseColor = material.diffuse;
    if (diffuseTexture.get_width() > 0) {
        float4 texColor = diffuseTexture.sample(textureSampler, in.texcoord);
        diffuseColor *= texColor.rgb;
    }
    float3 diffuse = lightData.lightColor * diffuseColor * NdotL;

    // Specular component
    float3 viewDir = normalize(-in.fragPos); // Assuming camera at origin
    float3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    float3 specular = lightData.lightColor * material.specular * spec;

    // Combine components
    float3 finalColor = ambient + diffuse + specular;
    finalColor = min(finalColor, float3(1.0)); // Ensure color stays within [0,1]

    return float4(finalColor, 1.0);
}
