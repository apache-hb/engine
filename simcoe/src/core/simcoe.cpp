#include "simcoe/simcoe.h"
#include "simcoe/core/logging.h"

using namespace simcoe;

namespace {
    logging::Category category(const char *pzName) {
        auto logger = logging::Category(logging::eInfo, pzName);
        logger.addSink(&gDebugSink);
        return logger;
    }
}

logging::DebugSink simcoe::gDebugSink = logging::DebugSink();

logging::Category simcoe::gLogs[simcoe::eTotal] = {
    category("general"),
    category("render")
};
