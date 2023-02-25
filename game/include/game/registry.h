#pragma once

#include "simcoe/core/util.h"

namespace game {
    namespace util = simcoe::util;

    struct DebugGui {
        const char *pzName;
        bool enabled = true;
    };

    extern util::Registry<DebugGui> debug;
}
