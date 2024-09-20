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
    float3 lightPosition;
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
    float3 normal = normalize(in.normal);

    float3 lightDir = lightData.lightPosition - in.fragPos;
    float distance = length(lightDir);
    lightDir = normalize(lightDir);

    float constantAttenuation = 1.0;
    float linearAttenuation = 0.001;  // Reduce this to make light falloff less drastic
    float quadraticAttenuation = 0.0001;  // Lower this even more for smooth falloff
    float attenuation = 1.0 / (constantAttenuation + linearAttenuation * distance + quadraticAttenuation * (distance * distance));

    // Ambient component
    float ambientStrength = 0.3;
    float3 ambient = lightData.ambientColor * material.ambient * ambientStrength;

    // Diffuse component
    float NdotL = max(dot(normal, lightDir), 0.0);
    float3 diffuseColor = material.diffuse;
    if (diffuseTexture.get_width() > 0) {
        float4 texColor = diffuseTexture.sample(textureSampler, in.texcoord);
        diffuseColor *= texColor.rgb;
    }
    float3 diffuse = lightData.lightColor * diffuseColor * NdotL * attenuation;

    // Specular component
    float3 viewDir = normalize(-in.fragPos);
    float3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    float3 specular = lightData.lightColor * material.specular * spec * attenuation;

    // Combine components
    float3 finalColor = ambient + diffuse + specular;
    finalColor = min(finalColor, float3(1.0)); 

    return float4(finalColor, 1.0);
}
