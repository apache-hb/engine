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
        Device(UINT index, Deadzone lstick, Deadzone rstick, int16_t ltrigger, int16_t rtrigger)
            : index(index) 
            , lstick(lstick)
            , rstick(rstick)
            , ltrigger(ltrigger)
            , rtrigger(rtrigger)
        { }

        bool valid() const;
        void vibrate(USHORT left, USHORT right);
        input::Input poll();
    private:
        UINT index;
        Deadzone lstick;
        Deadzone rstick;
        int16_t ltrigger;
        int16_t rtrigger;
        bool ok;
    };
}
