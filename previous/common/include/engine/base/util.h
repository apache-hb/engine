#pragma once

#include "engine/base/panic.h"

#include <string>
#include <string_view>
#include <sstream>
#include <span>

namespace simcoe {
    template<typename T>
    bool eq(const T* pLeft, const T* pRight, size_t count = 1) {
        return std::memcmp(pLeft, pRight, count * sizeof(T)) == 0;
    }

    namespace strings {
        std::string narrow(std::wstring_view str);
        std::wstring widen(std::string_view str);

        template<typename T>
        std::string join(std::span<T> items, std::string_view sep) {
            std::stringstream ss;
            for (size_t i = 0; i < items.size(); i++) {
                if (i != 0) {
                    ss << sep;
                }
                ss << items[i];
            }
            return ss.str();
        }
    }

    struct Timer {
        Timer();
        double tick();

    private:
        size_t last;
    };
}
