#include "win32.h"

export module engine.util.timer;

export namespace engine {
    struct Timer {
        Timer() = default;
        Timer(LARGE_INTEGER now, LARGE_INTEGER frequency)
            : now(now)
            , frequency(frequency)
        { }
        
        float tick() {
            LARGE_INTEGER then = now;
            QueryPerformanceCounter(&now);

            return float(now.QuadPart - then.QuadPart) / frequency.QuadPart;
        }

    private:
        LARGE_INTEGER now;
        LARGE_INTEGER frequency;
    };

    Timer createTimer() {
        LARGE_INTEGER frequency;
        LARGE_INTEGER start;
        QueryPerformanceFrequency(&frequency);
        QueryPerformanceCounter(&start);

        return { start, frequency };
    }
}
