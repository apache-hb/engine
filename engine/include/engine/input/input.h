#pragma once

#include "engine/math/math.h"

namespace engine::input {
    // type of input device
    enum Device {
        eDesktop,
        eGamepad
    };

    struct Action {
        enum Kind { eKeyDown, eKeyUp } kind;
    };

    struct Input {
        Device device = eDesktop;

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
