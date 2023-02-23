#pragma once

#include "simcoe/core/util.h"

namespace game {
    namespace util = simcoe::util;

    struct DebugGui;

    extern util::Registry<DebugGui> debug;
}
