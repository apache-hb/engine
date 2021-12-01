#include "win32.h"

#include <format>

namespace engine::win32 {
    Win32Error::Win32Error(std::string_view message) 
        : Error(std::format("(win32: 0x{:x}, msg: {})", GetLastError(), message))
    { }
}
