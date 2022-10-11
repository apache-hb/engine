#pragma once

#include "engine/math/math.h"

namespace engine::input {
    enum Device {
        eMouseAndKeyboard,
        eGamepad
    };

    struct Input {
        Device device = eMouseAndKeyboard;

        bool enableConsole;
        math::float3 movement;
        math::float2 rotation;
    };

    struct Gamepad {
        Gamepad(size_t index = 0);

        bool poll(Input *input);

    private:
        bool shouldSendPacket(size_t packet) const;
        void updatePacket(size_t newPacket);

        size_t index;
        size_t lastPacket = 0;
    };
}
