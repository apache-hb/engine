#pragma once

#include "engine/math/math.h"
#include "engine/base/win32.h"
#include "engine/base/util.h"

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

        enum Key {
            eW,
            eA,
            eS,
            eD,
        
            eE,
            eQ,

            eUnknown,

            eTotal
        };

        struct Info {
            // to get nice feeling keyboard input we assign priority to each keypress
            // the highest priority was the most recent key press
            // a value of 0 means the key isnt pressed
            size_t priority = 0;
            size_t pressed[eTotal] = { };
            
            size_t toggleConsole = SIZE_MAX;

            math::Vec2<int> mousePosition;

            // do we have new input data
            bool dirty = false;
        };

        friend LRESULT CALLBACK windowCallback(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

    private:
        math::Vec2<int> center();

        // actual window handle
        HWND hwnd;

        // when was the moust recent mouse event recorded
        Timer lastMouseEvent;

        Info info = { };
    };
}
