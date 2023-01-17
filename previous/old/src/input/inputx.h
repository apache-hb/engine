#pragma once

#include "util/win32.h"
#include "binds.h"

#include <xinput.h>

namespace engine::xinput {
    struct Deadzone {
        int16_t x;
        int16_t y;
    };

    struct Device {
        constexpr Device(UINT index, Deadzone lstick, Deadzone rstick, int16_t ltrigger, int16_t rtrigger) noexcept
            : index(index) 
            , lstick(lstick)
            , rstick(rstick)
            , ltrigger(ltrigger)
            , rtrigger(rtrigger)
        { }

        bool valid() const noexcept;
        void vibrate(USHORT left, USHORT right) noexcept;
        input::Input poll() noexcept;
    private:
        UINT index;
        Deadzone lstick;
        Deadzone rstick;
        int16_t ltrigger;
        int16_t rtrigger;
        bool ok = false;
    };
}
