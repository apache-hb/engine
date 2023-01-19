#pragma once

#include "simcoe/core/logging.h"

namespace simcoe {
    enum LogCategory {
        eGeneral,
        eRender,

        eTotal
    };

    extern logging::DebugSink gDebugSink;
    extern logging::Category gLogs[eTotal];
}
