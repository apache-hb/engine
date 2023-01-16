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
        IChannel(std::string_view name, Level level)
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

        std::string_view getName() const { return name; }

    protected:
        std::string_view name;
    };

    void init();
    IChannel &get(Category category = eGeneral);

    namespace detail {
        void info(Category category, std::string_view message);
        void warn(Category category, std::string_view message);
        void fatal(Category category, std::string_view message);
    }

    namespace v2 {
        using Channel = UniquePtr<IChannel>;
        using ChannelMap = std::unordered_map<Category, Channel>;

        enum struct ChannelTag : size_t {};

        extern ChannelMap channels;

        IChannel& get(Category category);
        ChannelTag add(Category category, Channel channel);
        void remove(Category category, ChannelTag id);

        void info(Category category, std::string_view message, auto&&... args) {
            detail::info(category,  std::vformat(message, std::make_format_args(args...)));
        }

        void warn(Category category, std::string_view message, auto&&... args) {
            detail::warn(category, std::vformat(message, std::make_format_args(args...)));
        }

        void fatal(Category category, std::string_view message, auto&&... args) {
            detail::fatal(category, std::vformat(message, std::make_format_args(args...)));
        }

        void info(std::string_view message, auto&&... args) {
            info(eGeneral, message, args...);       
        }

        void warn(std::string_view message, auto&&... args) {
            warn(eGeneral, message, args...);
        }

        void fatal(std::string_view message, auto&&... args) {
            fatal(eGeneral, message, args...);
        }
    }
}
