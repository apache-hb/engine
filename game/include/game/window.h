#pragma once

#include "simcoe/core/system.h"

#include "simcoe/input/desktop.h"
#include "simcoe/input/gamepad.h"

namespace game {
    namespace input = simcoe::input;
    namespace system = simcoe::system;
    
    struct Window final : system::Window {
        Window(input::Mouse& mouse, input::Keyboard& keyboard);
        bool poll();
        LRESULT onEvent(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) override;

        input::Mouse& mouse;
        input::Keyboard& keyboard;
    };
}
