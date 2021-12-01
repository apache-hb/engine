#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "error.h"

namespace engine::win32 {
    struct Win32Error : Error {
        Win32Error(std::string_view message);

        DWORD err() const { return error; }

    private:
        DWORD error;
    };

}
