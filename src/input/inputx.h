#pragma once

#include "util/win32.h"
#include <xinput.h>

namespace engine::xinput {
    struct Device {
        Device(UINT index) 
            : index(index) 
        { }

        bool valid() const;
        void vibrate(USHORT left, USHORT right);
        XINPUT_STATE poll();
    private:
        UINT index;
        bool ok;
    };
}
