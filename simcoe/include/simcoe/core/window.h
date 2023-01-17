#pragma once

#include "simcoe/core/math/math.h"

#include <windows.h>

namespace simcoe::platform {
    struct Window {
        using Size = math::Resolution<size_t>;

        enum Style {
            eWindow,
            eBorderless,

            eTotal
        };

        Window(const char *pzTitle, const Size& size, Style style = eWindow);
        ~Window();

        void restyle(Style style);

        bool poll();

        HWND getHandle() const { return hWindow; }

    private:
        static LRESULT CALLBACK callback(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

        bool bHasFocus = false;
        HWND hWindow;
    };
}
