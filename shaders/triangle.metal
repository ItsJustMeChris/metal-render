#include <metal_stdlib>
using namespace metal;

struct Uniforms {
    float2 scale;
};

vertex float4
vertexShader_triangle(uint vertexID [[vertex_id]],
             constant simd::float3* vertexPositions [[buffer(0)]],
             constant Uniforms &uniforms [[buffer(1)]])
{
    float3 scaledPosition = vertexPositions[vertexID] * float3(uniforms.scale, 1.0f);
    return float4(scaledPosition, 1.0f);
}

fragment float4 fragmentShader_triangle(float4 vertexOutPositions [[stage_in]]) {
    return float4(182.0f/255.0f, 240.0f/255.0f, 228.0f/255.0f, 1.0f);
}
