#pragma once

#include "win32.h"

namespace engine::util {
    struct Timer {
        Timer() = default;
        Timer(LARGE_INTEGER now, LARGE_INTEGER frequency)
            : now(now)
            , frequency(frequency)
        { }
        
        float tick();
    private:
        LARGE_INTEGER now;
        LARGE_INTEGER frequency;
    };

    Timer createTimer();
}
