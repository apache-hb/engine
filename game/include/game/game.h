#pragma once

#include "simcoe/core/system.h"

#include "simcoe/input/desktop.h"
#include "simcoe/input/gamepad.h"

#include "simcoe/locale/locale.h"

namespace game {
    namespace input = simcoe::input;
    namespace system = simcoe::system;

    struct Input {
        Input() : gamepad(0), keyboard(), mouse(false, true) {
            manager.add(&gamepad);
            manager.add(&keyboard);
            manager.add(&mouse);
        }

        void poll() {
            manager.poll();
        }

        input::Gamepad gamepad;
        input::Keyboard keyboard;
        input::Mouse mouse;

        input::Manager manager;
    };

    struct Info {
        system::Size windowResolution;
        system::Size renderResolution;

        simcoe::Locale locale;
    };
}
