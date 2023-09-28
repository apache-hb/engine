#include "simcoe/simcoe.h"
#include "simcoe/core/logging.h"

using namespace simcoe;

namespace {
    std::unordered_set<logging::ISink *> gSinks;

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

logging::Category simcoe::gLog = category("general");
logging::Category simcoe::gRenderLog = category("render");
logging::Category simcoe::gInputLog = category("input");
logging::Category simcoe::gAssetLog = category("asset");

void simcoe::addSink(logging::ISink *pSink) {
    gSinks.insert(pSink);
    gLog.addSink(pSink);
    gRenderLog.addSink(pSink);
    gInputLog.addSink(pSink);
}

void simcoe::removeSink(logging::ISink *pSink) {
    gSinks.erase(pSink);
    gLog.removeSink(pSink);
    gRenderLog.removeSink(pSink);
    gInputLog.removeSink(pSink);
}
