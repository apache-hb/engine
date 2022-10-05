#pragma once

#include "engine/base/io.h"

#include <format>
#include <span>

namespace engine::logging {
    enum Level {
        eDebug,
        eInfo,
        eWarn,
        eFatal,
        eAssert
    };

    struct Channel {
        Channel(std::string_view name, Level level)
            : level(level)
            , name(name)
        { }

        virtual ~Channel() = default;

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

    protected:
        std::string_view name;
    };

    struct IoChannel : Channel {
        IoChannel(std::string_view name, Io *io, Level level = eFatal)
            : Channel(name, level)
            , io(io)
        { }

    protected:
        virtual void send(Level reportLevel, const std::string_view message) override;

    private:
        Io *io;
    };

    struct ConsoleChannel : Channel {
        ConsoleChannel(std::string_view name, Level level = eInfo)
            : Channel(name, level)
        { }

    protected:
        virtual void send(Level reportLevel, const std::string_view message) override;
    };

    struct MultiChannel : Channel {
        MultiChannel(std::string_view name, std::span<Channel*> channels, Level level = eDebug)
            : Channel(name, level)
            , channels(channels)
        { }

    protected:
        virtual void send(Level reportLevel, const std::string_view message) override;

    private:
        std::span<Channel*> channels;
    };
}
