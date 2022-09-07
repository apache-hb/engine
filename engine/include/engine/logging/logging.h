#pragma once

#include "engine/base/io.h"

#include <fmt/format.h>

namespace engine::logging {
    enum Level {
        eInfo,
        eWarn,
        eFatal,
        eAssert
    };

    struct Channel {
        Channel(std::string_view name)
            : name(name)
        { }

        virtual ~Channel() = default;

        template<typename... A>
        void info(std::string_view message, A&&... args) {
            process(eInfo, fmt::vformat(message, fmt::make_format_args(args...)));
        }

        template<typename... A>
        void warn(std::string_view message, A&&... args) {
            process(eWarn, fmt::vformat(message, fmt::make_format_args(args...)));
        }

        template<typename... A>
        void fatal(std::string_view message, A&&... args) {
            process(eFatal, fmt::vformat(message, fmt::make_format_args(args...)));
        }

    protected:
        virtual void send(Level level, std::string_view message) = 0;

        std::string_view name;

    private:
        void process(Level level, std::string_view message);
    };

    struct IoChannel : Channel {
        IoChannel(std::string_view name, Io *io)
            : Channel(name)
            , io(io)
        { }

    protected:
        virtual void send(Level level, std::string message) override;

    private:
        Io *io;
    };
}
