#pragma once

namespace simcoe::math {
    template<typename T>
    constexpr T kPi { T(3.14159265358979323846) };

    template<typename T>
    constexpr T k2Pi { kPi<T> * 2 };

    template<typename T>
    constexpr T kPiDiv2 { kPi<T> / 2 };

    template<typename T>
    constexpr T kPiDiv4 { kPi<T> / 4 };
}
