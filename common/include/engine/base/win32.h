#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define DLLEXPORT __declspec(dllexport)

namespace simcoe::win32 {
    void init();

    void showCursor(bool show);
}
