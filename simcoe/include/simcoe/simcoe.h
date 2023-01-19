#pragma once

#include "simcoe/core/logging.h"

namespace simcoe {
    enum LogCategory {
        eGeneral,
        eRender,

        eTotal
    };

    extern logging::FileSink gFileSink;
    extern logging::ConsoleSink gConsoleSink;
    extern logging::DebugSink gDebugSink;
    extern logging::Category gLogs[eTotal];
}
