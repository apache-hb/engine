#pragma once

#include <format>

namespace logging {
    enum Level {
        INFO,
        WARN,
        FATAL
    };

    struct Channel {
        Channel(std::string name) : name(name) { }
        virtual ~Channel() { }

        void log(Level report, const std::string &message);

        template<typename... A>
        void info(const std::string &message, A&&... args) {
            log(INFO, std::vformat(message, std::make_format_args(args...)));
        }

        template<typename... A>
        void warn(const std::string &message, A&&... args) {
            log(WARN, std::vformat(message, std::make_format_args(args...)));
        }

        template<typename... A>
        void fatal(const std::string &message, A&&... args) {
            log(FATAL, std::vformat(message, std::make_format_args(args...)));
        }

        virtual void send(Level report, const std::string &message) = 0;
    
    protected:
        const std::string &channel() const { return name; }

    private:
        std::string name;
        Level level = INFO;
    };

    struct ConsoleChannel : Channel {
        ConsoleChannel(std::string name, FILE *file) 
            : Channel(name)
            , file(file) 
        { }

        virtual void send(Level report, const std::string &message) override;

        static void init();
    private:
        FILE *file;
    };
}
