#pragma once

namespace simcoe::input::detail {
    template<typename T>
    bool update(T& value, T next) {
        bool changed = value != next;
        value = next;
        return changed;
    }
}
