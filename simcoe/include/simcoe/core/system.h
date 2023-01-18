#pragma once

#include "simcoe/core/async/generator.h"
#include "simcoe/core/math/math.h"

#include <windows.h>

namespace simcoe::system {
    using StackTrace = async::Generator<const char*>;
    using Size = math::Resolution<size_t>;

    enum struct WindowStyle {
        eWindow,
        eBorderless,

        eTotal
    };

    struct Window {
        Window(const char *pzTitle, const Size& size, WindowStyle style = WindowStyle::eWindow);
        virtual ~Window();

        void restyle(WindowStyle style);

        bool poll();
        Size size();

        HWND getHandle() const { return hWindow; }

        virtual LRESULT onEvent(HWND, UINT, WPARAM, LPARAM) { return false; }

    private:
        static LRESULT CALLBACK callback(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

        bool bHasFocus = false;
        HWND hWindow;
    };

    void init();
    void deinit();

    StackTrace backtrace();
}
