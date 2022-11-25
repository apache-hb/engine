#pragma once

#include "engine/base/io/io.h"

#include <format>
#include <span>

namespace simcoe::logging {
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

        const char *getName() const { return name.data(); }

    protected:
        std::string_view name;
    };

    enum Category {
        eGeneral,
        eInput,
        eRender,
        eLocale,

        eTotal
    };

    void init();
    Channel &get(Category category = eGeneral);
}
