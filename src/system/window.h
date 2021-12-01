#pragma once

#include <windows.h>
#include <tuple>
#include "util/error.h"

namespace engine::win32 {
    struct WindowHandle {
        using Size = std::tuple<LONG, LONG>;

        WindowHandle() = delete;
        WindowHandle(const WindowHandle&) = delete;
        WindowHandle(WindowHandle&&) = delete;

        WindowHandle(
            HINSTANCE instance,
            int show,
            LPCTSTR title,
            LONG width,
            LONG height
        );
        virtual ~WindowHandle();

        void run();

        virtual void onCreate() { }
        virtual void onDestroy() { }

        virtual void onKeyPress(int key) { }
        virtual void onKeyRelease(int key) { }

        virtual void repaint() { }

        HWND getHandle() const { return handle; }
        RECT getClientRect() const;
        Size getClientSize() const;

    private:
        HINSTANCE instance;
        LPCTSTR name;
        HWND handle;
    };
}
