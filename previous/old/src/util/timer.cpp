#include "timer.h"

namespace engine::util {
    float Timer::tick() {
        LARGE_INTEGER then = now;
        QueryPerformanceCounter(&now);

        return float(now.QuadPart - then.QuadPart) / frequency.QuadPart;
    }

    Timer createTimer() {
        LARGE_INTEGER frequency;
        LARGE_INTEGER start;
        QueryPerformanceFrequency(&frequency);
        QueryPerformanceCounter(&start);

        return { start, frequency };
    }
}
