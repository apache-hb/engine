#pragma once

#include "engine/math/math.h"

#include <string>
#include <vector>

namespace engine::platform {
    struct Window {
        Window() = delete;
        Window(const Window&) = delete;
        Window(Window&&) = delete;

    private:
        
    };

    struct Display {
        Display() = delete;
        Display(std::string name, std::string desc, bool primary)
            : name(name)
            , desc(desc)
            , primary(primary)
        { }

        Window openWindow(std::string title, math::Resolution<size_t> size, math::Resolution<size_t> position);

        std::string name;
        std::string desc;
        bool primary;
    };

    struct System {
        System() noexcept;

        System(const System&) = delete;
        System(System&&) = delete;

        std::string name;
        std::vector<Display> displays;
    };
}
