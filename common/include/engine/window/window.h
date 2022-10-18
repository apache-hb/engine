#pragma once

#include "engine/math/math.h"
#include "engine/base/win32.h"
#include "engine/base/util.h"

#include "engine/input/input.h"

namespace simcoe {
    struct Window {
        Window(int width, int height, const char *title);
        ~Window();

        Window(const Window&) = delete;
        Window(Window&&) = delete;

        math::Resolution<int> size() const;
        HWND handle();

        size_t dpi() const;

        bool poll(input::Keyboard *pKeyboard);

        void imgui();

        std::bitset<256> keys;
    private:
        math::Vec2<int> center();

        // actual window handle
        HWND hwnd = nullptr;
    };
}
