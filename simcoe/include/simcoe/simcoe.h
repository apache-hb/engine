#pragma once

#include "simcoe/core/logging.h"

namespace simcoe {
    extern logging::FileSink gFileSink;
    extern logging::ConsoleSink gConsoleSink;
    extern logging::DebugSink gDebugSink;

    extern logging::Category gLog;
    extern logging::Category gRenderLog;
    extern logging::Category gInputLog;
}
