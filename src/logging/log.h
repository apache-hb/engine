#pragma once

#include <string_view>
#include <format>

namespace engine::logging {
    enum Level {
        INFO,
        WARN,
        FATAL
    };

    struct Channel {
        Channel(std::string_view name) : name(name) { }
        virtual ~Channel() { }

        void log(Level report, std::string_view message);

        template<typename... A>
        void info(std::string_view message, A&&... args) {
            log(INFO, std::vformat(message, std::make_format_args(args...)));
        }

        template<typename... A>
        void warn(std::string_view message, A&&... args) {
            log(WARN, std::vformat(message, std::make_format_args(args...)));
        }

        template<typename... A>
        void fatal(std::string_view message, A&&... args) {
            log(FATAL, std::vformat(message, std::make_format_args(args...)));
        }

        virtual void send(Level report, std::string_view message) = 0;
    
    protected:
        std::string_view channel() const { return name; }

    private:
        std::string_view name;
        Level level = INFO;
    };

    struct ConsoleChannel : Channel {
        ConsoleChannel(std::string_view name, FILE *file) 
            : Channel(name)
            , file(file) 
        { }

        virtual void send(Level report, std::string_view message) override;

        static void init();
    private:
        FILE *file;
    };
}
