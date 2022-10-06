#pragma once

#include "engine/math/math.h"
#include "engine/base/win32.h"

#include "engine/input/input.h"

namespace engine {
    struct Window {
        Window(int width, int height, const char *title);
        ~Window();

        Window(const Window&) = delete;
        Window(Window&&) = delete;

        math::Resolution<int> size() const;
        HWND handle();

        size_t dpi() const;

        bool poll(input::Input *input);

        void imguiNewFrame();

    private:
        HWND hwnd;
        bool running = true;
    };
}
