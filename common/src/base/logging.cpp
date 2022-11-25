#include "engine/base/logging.h"
#include "engine/base/io/io.h"
#include "engine/base/container/unique.h"
#include "engine/base/win32.h"
#include "vendor/imgui/imgui.h"

#include <ranges>
#include <iostream>
#include <array>

using namespace simcoe;
using namespace simcoe::logging;

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

struct IoChannel final : Channel {
    IoChannel(std::string_view name, std::string_view path, Level level = eFatal)
        : Channel(name, level)
        , io(Io::open(path, Io::eWrite))
    { }

protected:
    void send(Level reportLevel, const std::string_view message) override {
        io->write(std::format("[{}:{}]: {}\n", name, levelName(reportLevel), message));
    }

private:
    UniquePtr<Io> io;
};

struct ConsoleChannel final : Channel {
    ConsoleChannel(std::string_view name, Level level = eInfo)
        : Channel(name, level)
    { }

protected:
    void send(Level reportLevel, const std::string_view message) override {
        std::cout << std::format("[{}:{}]: {}", name, levelName(reportLevel), message) << std::endl;
    }
};

struct DebugChannel final : Channel {
    DebugChannel(std::string_view name, Level level = eInfo)
        : Channel(name, level)
    { }

protected:
    void send(Level reportLevel, const std::string_view message) override {
        OutputDebugStringA(std::format("[{}:{}]: {}\n", name, levelName(reportLevel), message).c_str());
    }
};

template<size_t N>
struct MultiChannel final : Channel {
    MultiChannel(std::string_view name, std::array<Channel*, N> channels, Level level = eDebug)
        : Channel(name, level)
        , channels(channels)
    { }

protected:
    void send(Level reportLevel, const std::string_view message) override {
        for (auto channel : channels) {
            channel->process(reportLevel, message);
        }
    }

private:
    std::array<Channel*, N> channels;
};

void Channel::process(Level reportLevel, std::string_view message) {
    if (level > reportLevel) { return; }

    for (const auto part : std::views::split(message, "\n")) {
        if (part.size() == 0) continue;
        
        send(reportLevel, std::string_view(part.begin(), part.end()));
    }
}

namespace {
    std::vector<Channel*> channels;

    Channel *addChannel(Channel *it) {
        channels.push_back(it);
        return it;
    }

    Channel *kSinks[eTotal];
}

void logging::init() {
    auto *fileLogger = addChannel(new IoChannel("general", "game.log"));
    auto *consoleLogger = addChannel(new ConsoleChannel("general", eDebug));
    auto *debugLogger = addChannel(new DebugChannel("general", eDebug));

    Channel *generalChannels[] { fileLogger, consoleLogger, debugLogger };

    kSinks[eGeneral] = addChannel(new MultiChannel<3>("general", std::to_array(generalChannels)));
    kSinks[eInput] = kSinks[eGeneral];
    kSinks[eRender] = kSinks[eGeneral];
    kSinks[eLocale] = kSinks[eGeneral];
}

Channel &logging::get(Category category) {
    return *kSinks[category];
}
