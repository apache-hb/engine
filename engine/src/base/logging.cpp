#include "engine/base/logging.h"

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

void Channel::process(Level reportLevel, std::string_view message) {
    if (level > reportLevel) { return; }

    for (const auto part : std::views::split(message, "\n")) {
        send(reportLevel, std::string_view(part.begin(), part.end()));
    }
}

void IoChannel::send(Level reportLevel, const std::string_view message) {
    io->write(fmt::format("[{}:{}]: {}\n", name, levelName(reportLevel), message));
}

void ConsoleChannel::send(Level reportLevel, const std::string_view message) {
    fmt::print("[{}:{}]: {}\n", name, levelName(reportLevel), message);
}

void MultiChannel::send(Level reportLevel, const std::string_view message) {
    for (auto channel : channels) {
        channel->process(reportLevel, message);
    }
}
