#include "simcoe/simcoe.h"
#include "simcoe/core/logging.h"

using namespace simcoe;

namespace {
    logging::Category category(const char *pzName) {
        auto logger = logging::Category(logging::eInfo, pzName);
        logger.addSink(&gFileSink);
        logger.addSink(&gConsoleSink);
        logger.addSink(&gDebugSink);
        return logger;
    }
}

logging::FileSink simcoe::gFileSink = logging::FileSink("file", "simcoe.log");
logging::ConsoleSink simcoe::gConsoleSink = logging::ConsoleSink("console");
logging::DebugSink simcoe::gDebugSink = logging::DebugSink();

logging::Category simcoe::gLogs[simcoe::eTotal] = {
    category("general"),
    category("render")
};
