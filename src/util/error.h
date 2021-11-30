#pragma once

#include <string>
#include <string_view>

namespace engine {
    struct Error {
        Error(std::string message) : message(message) { }
        std::string_view msg() const { return message; }

        virtual std::string string() const { return message; }

    private:
        std::string message;
    };
}
