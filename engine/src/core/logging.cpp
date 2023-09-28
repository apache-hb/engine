#include "simcoe/core/logging.h"
#include "simcoe/core/win32.h"
#include "simcoe/core/panic.h"
#include <ranges>

using namespace simcoe;
using namespace simcoe::logging;

namespace {
    struct LevelFormat {
        const char *pzLevelName;
        const char *pzAnsiColour;
    };

    constexpr const char *kpzAnsiReset = "\x1b[0m";

    constexpr LevelFormat getLevelFormat(Level level) {
        switch (level) {
        case eInfo: return { "info", "\x1b[32m" };
        case eWarn: return { "warn", "\x1b[33m" };
        case eFatal: return { "fatal", "\x1b[31m" };
        default: return { "unknown", "\x1b[36m" };
        }
    }
}

void Category::send(Level level, const char *pzMessage) {
    for (ISink *pSink : sinks) {
        pSink->send(*this, level, pzMessage);
    }
}

void Category::addSink(ISink *pSink) {
    sinks.insert(pSink);
}

void Category::removeSink(ISink *pSink) {
    sinks.erase(pSink);
}

void IFilterSink::send(Category &category, Level level, const char *pzMessage) {
    if (level < category.getLevel()) {
        return;
    }

    accept(category, level, pzMessage);
}

ConsoleSink::ConsoleSink(const char *pzName)
    : IFilterSink(pzName) 
{ }

void ConsoleSink::accept(Category &category, Level level, const char *pzMessage) {
    const auto& [name, colour] = getLevelFormat(level);

    printf("[%s%s:%s%s] %s%s\n", colour, category.getName(), name, kpzAnsiReset, pzMessage, kpzAnsiReset);
}

FileSink::FileSink(const char *pzName, const char *pzPath): IFilterSink(pzName) { 
    errno_t err = fopen_s(&pFile, pzPath, "w");
    
    if (err != 0) {
        err = fopen_s(&pFile, "\\\\.\\NUL", "a");
    }

    ASSERT(err == 0);
}

void FileSink::accept(Category &category, Level level, const char *pzMessage) {
    const auto& [name, _] = getLevelFormat(level);

    fprintf(pFile, "[%s:%s] %s\n", category.getName(), name, pzMessage);
}

void DebugSink::accept(Category &category, Level level, const char *pzMessage) {
    const auto& [name, _] = getLevelFormat(level);

    OutputDebugStringA(std::format("[{}:{}] {}\n", category.getName(), name, pzMessage).c_str());
}
