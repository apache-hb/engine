#pragma once

namespace simcoe::input::detail {
    template<typename T>
    bool update(T& value, T next) {
        value = next;
        return next != 0;
    }
}
