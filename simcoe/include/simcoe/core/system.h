#pragma once

#include "simcoe/async/generator.h"
#include "simcoe/math/math.h"
#include "simcoe/core/win32.h"

#define HR_CHECK(expr) \
    do { \
        if (HRESULT hr = (expr); FAILED(hr)) { \
            PANIC("{} = {}", #expr, simcoe::system::hrString(hr)); \
        } \
    } while (false) 

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

    std::string hrString(HRESULT hr);
    std::string win32String(DWORD dw);
    std::string win32LastErrorString();

    void showCursor(bool show);
}