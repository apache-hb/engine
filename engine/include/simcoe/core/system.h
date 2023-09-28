#pragma once

#include "simcoe/math/math.h"
#include "simcoe/core/win32.h"

#include <vector>

#define HR_CHECK(expr) \
    do { \
        if (HRESULT err = (expr); FAILED(err)) { \
            PANIC("{} = {}", #expr, simcoe::system::hrString(err)); \
        } \
    } while (false)

namespace simcoe::system {
    using Size = math::Resolution<size_t>;

    enum struct WindowStyle {
        eWindow,
        eBorderless,

        eTotal
    };

    struct Window {
        Window(const char *pzTitle, const Size& size, WindowStyle style = WindowStyle::eBorderless);
        virtual ~Window();

        void restyle(WindowStyle style);

        void hideCursor(bool hide);

        bool poll();
        Size size();
        float dpi();

        HWND getHandle() const { return hWindow; }

        virtual LRESULT onEvent(HWND, UINT, WPARAM, LPARAM) { return false; }

    private:
        static LRESULT CALLBACK callback(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

        bool bHideCursor = false;
        bool bHasFocus = false;
        HWND hWindow;
    };

    struct System {
        System();
        ~System();
    };

    struct Timer {
        Timer();
        float tick();

    private:
        size_t last;
    };

    std::vector<std::string> backtrace();

    std::string hrString(HRESULT hr);
    std::string win32String(DWORD dw);
    std::string win32LastErrorString();
}
