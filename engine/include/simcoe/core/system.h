#pragma once

#include "simcoe/math/math.h"
#include "simcoe/core/win32.h"

#include <vector>

#define HR_CHECK(expr) \
    do { \
        if (HRESULT err = (expr); FAILED(err)) { \
            PANIC("{} = {}", #expr, simcoe::os::hrString(err)); \
        } \
    } while (false)

namespace simcoe::os {
    using Size = math::Resolution<size_t>;

    enum struct WindowStyle {
        eWindow,
        eBorderless,

        eTotal
    };

    struct IWindowEvents {
        virtual ~IWindowEvents() = default;

        /**
         * return true if the event was handled
         */
        virtual bool onEvent(HWND, UINT, WPARAM, LPARAM) = 0;

        virtual void onClose() { }
        virtual void onOpen() { }

        virtual void onGainFocus() { }
        virtual void onLoseFocus() { }
    };

    struct WindowInfo {
        std::string_view title;
        Size size;
        WindowStyle style;
        IWindowEvents *pEvents;
    };

    struct Window final {
        friend struct System;

        ~Window();

        void restyle(WindowStyle style);
        void hideCursor(bool hide);

        float getDpi() const;
        Size getSize() const;
        HWND getHandle() const;
        IWindowEvents* getEvents() const;

    private:
        Window(HINSTANCE hInstance, int nCmdShow, const WindowInfo& info);

        static LRESULT CALLBACK callback(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

        bool bHideCursor = false;

        HWND hWindow;
        IWindowEvents* events;
    };

    struct System final {
        System(HINSTANCE hInstance);
        ~System();

        Window openWindow(int nCmdShow, const WindowInfo& info);
        bool poll();

    private:
        HINSTANCE hInstance;
    };

    struct Timer {
        Timer();
        float tick();

    private:
        size_t last;
    };

    std::string hrString(HRESULT hr);
    std::string win32String(DWORD dw);
    std::string win32LastErrorString();
}
