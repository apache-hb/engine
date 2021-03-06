#pragma once

#include <string_view>
#include <format>

#include <stdarg.h>
#include <windows.h>

namespace engine::log {
    enum Level {
        INFO,
        WARN,
        FATAL,
        TOTAL
    };

    struct Channel {
        Channel(std::string_view name, bool sanitize) 
            : name(name) 
            , sanitize(sanitize)
        { }

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
        virtual void tick() { }
        std::string_view getName() const { return name; }

    protected:
        std::string_view channel() const { return name; }

    private:
        void sendData(Level report, std::string_view message);
        
        std::string_view name;
        bool sanitize;
        Level level = INFO;
    };

    struct ConsoleChannel : Channel {
        ConsoleChannel(std::string_view name, FILE* file) 
            : Channel(name, true)
            , file(file) 
        { }

        virtual void send(Level report, std::string_view message) override;

        static DWORD init();
    private:
        FILE* file;
    };

    extern Channel* global;
    extern Channel* loader;
    extern Channel* render;
    extern Channel* discord;
}
