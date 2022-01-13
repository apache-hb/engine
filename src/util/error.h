#pragma once

#include "logging/log.h"

#include <string>
#include <format>
#include <string_view>
#include <source_location>

namespace engine {
    struct Error {
        Error(std::string message = "", std::source_location location = std::source_location::current())
            : message(message)
            , location(location)
        {
            log::global->warn("exception: {}", message);
        }

        virtual std::string query() const { return std::string(message); }

        std::string_view what() const { return message; }
        std::source_location where() const { return location; }

        auto line() const { return location.line(); }
        auto column() const { return location.column(); }

        auto file() const { return location.file_name(); }
        auto function() const { return location.function_name(); }
    private:
        std::string message;
        std::source_location location;
    };

    struct Errno : Error {
        Errno(errno_t code, std::string message = "", std::source_location location = std::source_location::current())
            : Error(message, location)
            , code(code)
        { }

        virtual std::string query() const override {
            return std::format("errno: {}", code);
        }

        errno_t error() const { return code; }

    private:
        errno_t code;
    };
}
