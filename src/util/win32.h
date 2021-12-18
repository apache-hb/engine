#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "error.h"

#include <format>

namespace engine::win32 {
    std::string toString(DWORD err);
    
    struct Error : engine::Error {
        Error(std::string message = "", std::source_location location = std::source_location::current())
            : Error(GetLastError(), message, location)
        { }

        Error(DWORD code, std::string message = "", std::source_location location = std::source_location::current())
            : engine::Error(message, location)
            , code(code)
        { }

        virtual std::string query() const { return std::format("win32(%d) %s", error(), what()); }

        DWORD error() const { return code; }

    private:
        DWORD code;
    };
}
