#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "error.h"

namespace engine::win32 {
    template<typename T>
    using Result = engine::Result<T, DWORD>;
}
