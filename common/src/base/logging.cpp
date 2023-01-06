#include "engine/base/logging.h"
#include "engine/base/io/io.h"
#include "engine/base/container/unique.h"
#include "engine/base/win32.h"

#include "engine/base/memory/slotmap.h"

#include "vendor/imgui/imgui.h"

#include <ranges>
#include <iostream>
#include <array>

using namespace simcoe;
using namespace simcoe::logging;

namespace {
    struct CategoryInfo {
        Category category;
        std::string_view name;
    };

    constexpr auto kCategoryInfo = std::to_array<CategoryInfo>({
        { eGeneral, "general" },
        { eInput, "input" },
        { eRender, "render" },
        { eLocale, "locale" },
    });

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
    IoChannel(std::string_view name, std::string_view path, Level level = eFatal)
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
    ConsoleChannel(std::string_view name, Level level = eInfo)
        : IChannel(name, level)
    { }

protected:
    void send(Level reportLevel, const std::string_view message) override {
        std::cout << std::format("[{}:{}]: {}\n", name, levelName(reportLevel), message);
    }
};

struct DebugChannel final : IChannel {
    DebugChannel(std::string_view name, Level level = eInfo)
        : IChannel(name, level)
    { }

protected:
    void send(Level reportLevel, const std::string_view message) override {
        OutputDebugStringA(std::format("[{}:{}]: {}\n", name, levelName(reportLevel), message).c_str());
    }
};

struct MultiChannel final : IChannel {
    MultiChannel(std::string_view name, std::vector<IChannel*> input, Level level = eDebug)
        : IChannel(name, level)
        , channels(16)
    { 
        for (auto channel : input) {
            channels.alloc(channel);
        }
    }

    v2::ChannelTag add(IChannel *channel) {
        return static_cast<v2::ChannelTag>(channels.alloc(channel));
    }

    void remove(v2::ChannelTag tag) {
        channels.update(size_t(tag), nullptr);
    }

protected:
    void send(Level reportLevel, const std::string_view message) override {
        for (auto channel : channels) {
            channel->process(reportLevel, message);
        }
    }

private:
    using IndexMap = memory::AtomicSlotMap<IChannel*>;

    IndexMap channels;
};

void IChannel::process(Level reportLevel, std::string_view message) {
    if (level > reportLevel) { return; }

    for (const auto part : std::views::split(message, "\n")) {
        if (part.size() == 0) continue;
        
        send(reportLevel, std::string_view(part.begin(), part.end()));
    }
}

void logging::init() {
    
}

namespace simcoe::logging::detail {
    void info(Category category, std::string_view message) {
        logging::v2::get(category).process(eInfo, message);
    }

    void warn(Category category, std::string_view message) {
        logging::v2::get(category).process(eWarn, message);
    }

    void fatal(Category category, std::string_view message) {
        logging::v2::get(category).process(eFatal, message);
    }
}

namespace simcoe::logging::v2 {
    ChannelMap channels = []() -> ChannelMap {
        ChannelMap result;

        auto *file = new IoChannel("general", "game.log");
        auto *console = new ConsoleChannel("general", eDebug);
        auto *debug = new DebugChannel("general", eDebug);

        for (auto [category, name] : kCategoryInfo) {
            result.emplace(category, new MultiChannel(name, { file, console, debug }));
        }

        return result;
    }();

    IChannel& get(Category category) {
        return *channels[category].get();
    }

    ChannelTag add(Category category, IChannel *channel) {
        auto *it = static_cast<MultiChannel*>(channels[category].get());
        return it->add(channel);
    }

    void remove(Category category, ChannelTag tag) {
        auto *it = static_cast<MultiChannel*>(channels[category].get());
        it->remove(tag);
    }
}
