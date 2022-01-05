#pragma once

namespace engine::math {
    template<typename T>
    struct Resolution {
        T width;
        T height;

        float aspectRatio() const {
            return float(width) / float(height);
        }
    };

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
}
