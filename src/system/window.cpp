#include "system.h"

namespace {
    LRESULT CALLBACK WindowHandleCallback(
        HWND hwnd,
        UINT msg,
        WPARAM wparam,
        LPARAM lparam
    )
    {
        auto *self = reinterpret_cast<engine::system::Window::Callbacks*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

        switch (msg) {
        case WM_CREATE: {
            LPCREATESTRUCT create = reinterpret_cast<LPCREATESTRUCT>(lparam);
            SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(create->lpCreateParams));
            return 0;
        }

        case WM_KEYDOWN:
            self->onKeyPress(static_cast<int>(wparam));
            return 0;

        case WM_KEYUP:
            self->onKeyRelease(static_cast<int>(wparam));
            return 0;

        case WM_PAINT:
            self->repaint();
            return 0;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;

        default:
            return DefWindowProc(hwnd, msg, wparam, lparam);
        }
    }
}

namespace engine::system {
    win32::Result<RECT> Window::getClientRect() const {
        RECT rect;
        if (!GetClientRect(handle, &rect)) {
            return fail(GetLastError());
        }
        return pass(rect);
    }

    win32::Result<Window::Size> Window::getClientSize() const {
        auto result = getClientRect();
        if (!result) { return result.forward(); }

        auto rect = result.value();
        auto width = rect.right - rect.left;
        auto height = rect.bottom - rect.top;

        return pass(Window::Size(width, height));
    }

    DWORD Window::run(int show) {
        if (ShowWindow(handle, show) != 0) {
            return GetLastError();
        }

        getCallbacks()->onCreate(this);
    
        MSG msg = { };
        while (msg.message != WM_QUIT) {
            if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }

        getCallbacks()->onDestroy();

        return 0;
    }

    win32::Result<Window> createWindow(HINSTANCE instance, LPCTSTR name, Window::Size size, Window::Callbacks *callbacks) {
        WNDCLASSEX wc = {
            .cbSize = sizeof(WNDCLASSEX),
            .style = CS_HREDRAW | CS_VREDRAW,
            .lpfnWndProc = WindowHandleCallback,
            .hInstance = instance,
            .hCursor = LoadCursor(NULL, IDC_ARROW),
            .lpszClassName = name
        };

        auto [width, height] = size;
        RECT rect = { .right = width, .bottom = height };
        
        if (RegisterClassEx(&wc) == 0) {
            return fail(GetLastError());
        }

        if (AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, false) == 0) {
            return fail(GetLastError());
        }

        HWND handle = CreateWindow(
            name, name, 
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, CW_USEDEFAULT,
            width, height,
            nullptr, nullptr,
            instance, callbacks
        );

        if (handle == nullptr) {
            return fail(GetLastError());
        }

        return pass(Window(instance, name, handle, callbacks));
    }

    void destroyWindow(Window &window) {
        DestroyWindow(window.getHandle());
        UnregisterClass(window.getName(), window.getInstance());
    }
}
