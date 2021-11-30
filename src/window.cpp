#include "window.h"

#include <format>

namespace {
    LRESULT CALLBACK WindowHandleCallback(
        HWND hwnd,
        UINT msg,
        WPARAM wparam,
        LPARAM lparam
    )
    {
        auto *self = reinterpret_cast<engine::WindowHandle*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
        LPCREATESTRUCT create;

        switch (msg)
        {
        case WM_CREATE:
            create = reinterpret_cast<LPCREATESTRUCT>(lparam);
            SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(create->lpCreateParams));
            return 0;

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
            self->onDestroy();
            PostQuitMessage(0);
            return 0;

        default:
            return DefWindowProc(hwnd, msg, wparam, lparam);
        }
    }
}

namespace engine {
    Win32Error::Win32Error(std::string_view message) 
        : Error(std::format("(win32: 0x{:x}, msg: {})", GetLastError(), message))
    { }

    WindowHandle::WindowHandle(
        HINSTANCE instance,
        int show,
        LPCTSTR title,
        LONG width,
        LONG height
    ) : instance(instance), name(title) {
        WNDCLASSEX wc = {
            .cbSize = sizeof(WNDCLASSEX),
            .style = CS_HREDRAW | CS_VREDRAW,
            .lpfnWndProc = WindowHandleCallback,
            .hInstance = instance,
            .hCursor = LoadCursor(NULL, IDC_ARROW),
            .lpszClassName = name
        };

        if (RegisterClassEx(&wc) == 0) {
            throw Win32Error("register-class");
        }

        RECT rect = {
            .right = width,
            .bottom = height
        };

        if (!AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, false)) {
            throw Win32Error("adjust-window-rect");
        }

        handle = CreateWindow(
            name, name, 
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, CW_USEDEFAULT,
            width, height,
            nullptr, nullptr,
            instance, this
        );

        if (handle == nullptr) {
            throw Win32Error("create-window");
        }

        if (ShowWindow(handle, show) != 0) {
            throw Win32Error("show-window");
        }
    }

    WindowHandle::~WindowHandle() {
        DestroyWindow(handle);
        UnregisterClass(name, instance);
    }

    void WindowHandle::run() {
        onCreate();

        MSG msg = { };
        while (msg.message != WM_QUIT) {
            if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
    }

    RECT WindowHandle::getClientRect() const {
        RECT rect;
        if (!GetClientRect(handle, &rect)) {
            throw Win32Error("get-client-rect");
        }
        return rect;
    }

    WindowHandle::Size WindowHandle::getClientSize() const {
        RECT rect = getClientRect();
        return WindowHandle::Size(rect.right - rect.left, rect.bottom - rect.top);
    }
}
