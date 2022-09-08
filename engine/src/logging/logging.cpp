#include "engine/logging/logging.h"

#include <ranges>

using namespace engine::logging;

namespace {
    constexpr std::string_view levelName(Level level) {
        switch (level) {
        case eInfo: return "info";
        case eWarn: return "warn";
        case eFatal: return "fatal";
        case eAssert: return "assert";
        default: return "unknown";
        }
    }
}

void Channel::process(Level level, std::string_view message) {
    for (const auto part : std::views::split(message, "\n")) {
        send(level, std::string_view(part.begin(), part.end()));
    }
}

void IoChannel::send(Level level, const std::string_view message) {
    io->write(fmt::format("[{}:{}]: {}\n", name, levelName(level), message));
}
