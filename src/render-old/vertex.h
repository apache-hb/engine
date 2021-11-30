#pragma once

namespace render {
    template<typename T>
    struct Vec3 {
        T x;
        T y;
        T z;
    };

    template<typename T>
    struct Vec4 {
        T x;
        T y;
        T z;
        T w;
    };

    using float3 = Vec3<float>;
    using float4 = Vec4<float>;

    struct Vertex {
        float3 colour;
        float4 position;
    };
}