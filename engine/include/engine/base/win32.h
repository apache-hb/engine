#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define DLLEXPORT __declspec(dllexport)

namespace engine::win32 {
    void init();
}
