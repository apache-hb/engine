#pragma once

#include "simcoe/core/system.h"

#include "simcoe/core/logging.h"

#include "simcoe/input/desktop.h"
#include "simcoe/input/gamepad.h"

#include "simcoe/locale/locale.h"

namespace game {
    namespace input = simcoe::input;
    namespace system = simcoe::system;
    namespace logging = simcoe::logging;

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

    struct GuiSink final : logging::ISink {
        GuiSink() : logging::ISink("gui") {}

        void send(logging::Category &category, logging::Level level, const char *pzMessage) override;
    
        struct Entry {
            logging::Level level;
            const char *pzCategory;
            std::string message;
        };

        std::unordered_set<const char*> categories;
        std::vector<Entry> entries;
    };

    struct Info {
        system::Size windowResolution;
        system::Size renderResolution;

        simcoe::Locale locale;

        GuiSink sink;
    };
}
