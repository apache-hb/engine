#pragma once

#include "engine/window/window.h"

namespace engine::ui {
    struct Create {
        const char *title;
        math::Resolution<int> size;

        const char *imgui = "imgui.ini";
    };

    engine::Window *init(const Create& create);
}
