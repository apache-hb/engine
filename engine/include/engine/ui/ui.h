#pragma once

#include "engine/window/window.h"

namespace simcoe::ui {
    struct Create {
        const char *title;
        math::Resolution<int> size;

        const char *imgui = "imgui.ini";
    };

    simcoe::Window *init(const Create& create);
}
