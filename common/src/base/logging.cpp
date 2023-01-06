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

struct IoChannel final : IChannel {
    IoChannel(const char* name, std::string_view path, Level level = eFatal)
        : IChannel(name, level)
        , io(Io::open(path, Io::eWrite))
    { }

protected:
    void send(Level reportLevel, const std::string_view message) override {
        io->write(std::format("[{}:{}]: {}\n", name, levelName(reportLevel), message));
    }

private:
    UniquePtr<Io> io;
};

struct ConsoleChannel final : IChannel {
    ConsoleChannel(const char* name, Level level = eInfo)
        : IChannel(name, level)
    { }

protected:
    void send(Level reportLevel, const std::string_view message) override {
        std::cout << std::format("[{}:{}]: {}\n", name, levelName(reportLevel), message);
    }
};

struct DebugChannel final : IChannel {
    DebugChannel(const char* name, Level level = eInfo)
        : IChannel(name, level)
    { }

protected:
    void send(Level reportLevel, const std::string_view message) override {
        OutputDebugStringA(std::format("[{}:{}]: {}\n", name, levelName(reportLevel), message).c_str());
    }
};

template<size_t N>
struct MultiChannel final : IChannel {
    MultiChannel(const char* name, std::array<IChannel*, N> channels, Level level = eDebug)
        : IChannel(name, level)
        , channels(channels)
    { }

protected:
    void send(Level reportLevel, const std::string_view message) override {
        for (auto channel : channels) {
            channel->process(reportLevel, message);
        }
    }

private:
    std::array<IChannel*, N> channels;
};

void IChannel::process(Level reportLevel, std::string_view message) {
    if (level > reportLevel) { return; }

    for (const auto part : std::views::split(message, "\n")) {
        if (part.size() == 0) continue;
        
        send(reportLevel, std::string_view(part.begin(), part.end()));
    }
}

namespace {
    auto *kFileChannel = new IoChannel("general", "game.log");
}

logging::ChannelMap logging::channels = {
    { eGeneral, }
};

void logging::init() {
    auto *fileLogger = addChannel(new IoChannel("general", "game.log"));
    auto *consoleLogger = addChannel(new ConsoleChannel("general", eDebug));
    auto *debugLogger = addChannel(new DebugChannel("general", eDebug));

    IChannel *generalChannels[] { fileLogger, consoleLogger, debugLogger };

    kSinks[eGeneral] = addChannel(new MultiChannel<3>("general", std::to_array(generalChannels)));
    kSinks[eInput] = kSinks[eGeneral];
    kSinks[eRender] = kSinks[eGeneral];
    kSinks[eLocale] = kSinks[eGeneral];
}

IChannel &logging::get(Category category) {
    return *channels[category].get();
}
