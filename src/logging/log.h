#pragma once

#include <string_view>
#include <format>

#include <stdarg.h>
#include <windows.h>

namespace engine::logging {
    enum Level {
        INFO,
        WARN,
        FATAL
    };

    struct Channel {
        Channel(std::string_view name, bool sanitize) 
            : name(name) 
            , sanitize(sanitize)
        { }

        virtual ~Channel() { }

        void log(Level report, std::string_view message);
        
        template<typename F>
        Channel *ensure(bool success, std::string_view message, F &&func) {
            if (!success) { 
                fatal("assert: {}", message);
                func();
            }
            return this;
        }

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

        void infof(const char *message, ...);
        void warnf(const char *message, ...);
        void fatalf(const char *message, ...);

        virtual void send(Level report, std::string_view message) = 0;
    
    protected:
        std::string_view channel() const { return name; }

    private:
        bool sanitize;
        std::string_view name;
        Level level = INFO;
    };

    struct ConsoleChannel : Channel {
        ConsoleChannel(std::string_view name, FILE *file) 
            : Channel(name, true)
            , file(file) 
        { }

        virtual void send(Level report, std::string_view message) override;

        static DWORD init();
    private:
        FILE *file;
    };
}
