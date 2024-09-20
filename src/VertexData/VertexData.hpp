#pragma once

#include <simd/simd.h>

struct VertexData
{
    simd::float4 position;
    simd::float3 normal;
    simd::float2 texcoord;

    bool operator==(const VertexData &other) const
    {
        return memcmp(this, &other, sizeof(VertexData)) == 0;
    }
} __attribute__((aligned(16)));

namespace std
{
    template <>
    struct hash<VertexData>
    {
        size_t operator()(const VertexData &v) const
        {
            size_t h = 0;
            for (int i = 0; i < 4; ++i)
            {
                h ^= std::hash<float>{}(v.position[i]) + 0x9e3779b9 + (h << 6) + (h >> 2);
            }
            for (int i = 0; i < 3; ++i)
            {
                h ^= std::hash<float>{}(v.normal[i]) + 0x9e3779b9 + (h << 6) + (h >> 2);
            }
            for (int i = 0; i < 2; ++i)
            {
                h ^= std::hash<float>{}(v.texcoord[i]) + 0x9e3779b9 + (h << 6) + (h >> 2);
            }
            return h;
        }
    };
}
