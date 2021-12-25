#pragma once

namespace engine::math {
    template<typename T>
    struct Vec3 {
        T x;
        T y;
        T z;
    };

    using float3 = Vec3<float>;
    using index = DWORD;
}
