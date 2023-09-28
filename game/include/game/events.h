#pragma once

#include "simcoe/core/system.h"

#include "simcoe/input/desktop.h"
#include "simcoe/input/gamepad.h"

namespace game {
    namespace input = simcoe::input;
    namespace os = simcoe::os;

    struct GameEvents final : os::IWindowEvents {
        GameEvents(input::Keyboard& keyboard)
            : keyboard(keyboard)
        { }

        bool onEvent(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) override;

        input::Keyboard& keyboard;
    };
}
