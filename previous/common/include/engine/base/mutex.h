#pragma once

#include "engine/base/container/handle.h"

namespace simcoe::threads {
    struct Mutex {
        Mutex();

        void wait();
        void signal();

    private:
        UniqueHandle event;
    };
}
