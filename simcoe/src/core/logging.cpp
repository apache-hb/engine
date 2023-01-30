#include "simcoe/core/logging.h"

#include <windows.h>

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
    if (level < getLevel()) {
        return;
    }

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

ConsoleSink::ConsoleSink(const char *pzName)
    : ISink(pzName) 
{ }

void ConsoleSink::send(Category &category, Level level, const char *pzMessage) {
    const auto& [name, colour] = getLevelFormat(level);

    printf("[%s%s:%s%s] %s%s\n", colour, category.getName(), name, kpzAnsiReset, pzMessage, kpzAnsiReset);
}

FileSink::FileSink(const char *pzName, const char *pzPath): ISink(pzName) { 
    auto err = fopen_s(&pFile, pzPath, "w");
    (void)err; // TODO: handle these
}

void FileSink::send(Category &category, Level level, const char *pzMessage) {
    const auto& [name, _] = getLevelFormat(level);

    fprintf(pFile, "[%s:%s] %s\n", category.getName(), name, pzMessage);
}

void DebugSink::send(Category &category, Level level, const char *pzMessage) {
    const auto& [name, _] = getLevelFormat(level);

    OutputDebugStringA(std::format("[{}:{}] {}\n", category.getName(), name, pzMessage).c_str());
}
