#pragma once

#include <numbers>

namespace simcoe::math {
    template<typename T>
    constexpr T kPi = std::numbers::pi_v<T>;

    template<typename T>
    constexpr T k2Pi { kPi<T> * 2 };

    template<typename T>
    constexpr T kPiDiv2 { kPi<T> / 2 };

    template<typename T>
    constexpr T kPiDiv4 { kPi<T> / 4 };
}
