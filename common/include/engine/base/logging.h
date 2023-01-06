#pragma once

#include "engine/base/io/io.h"

#include "engine/base/container/unique.h"

#include <format>
#include <span>
#include <unordered_map>

/**
 * logging
 * 
 * N categories -> M channels
 * 
 * categories control which channels messages are sent to
 * channels are responsible for formatting, filtering, and sending messages
 */
namespace simcoe::logging {
    enum Level {
        eDebug,
        eInfo,
        eWarn,
        eFatal,
        eAssert
    };

    enum Category {
        eGeneral,
        eInput,
        eRender,
        eLocale,

        eTotal
    };

    struct IChannel {
        IChannel(const char* name, Level level)
            : level(level)
            , name(name)
        { }

        virtual ~IChannel() = default;

        template<typename... A>
        void debug(std::string_view message, A&&... args) {
            process(eDebug, std::vformat(message, std::make_format_args(args...)));
        }

        template<typename... A>
        void info(std::string_view message, A&&... args) {
            process(eInfo, std::vformat(message, std::make_format_args(args...)));
        }

        template<typename... A>
        void warn(std::string_view message, A&&... args) {
            process(eWarn, std::vformat(message, std::make_format_args(args...)));
        }

        template<typename... A>
        void fatal(std::string_view message, A&&... args) {
            process(eFatal, std::vformat(message, std::make_format_args(args...)));
        }

        void process(Level reportLevel, std::string_view message);
        virtual void send(Level reportLevel, std::string_view message) = 0;

        Level level;

        const char *getName() const { return name; }

    protected:
        const char* name;
    };

    using ChannelMap = std::unordered_map<Category, UniquePtr<IChannel>>;

    extern ChannelMap channels;

    void init();
    IChannel &get(Category category = eGeneral);
}
