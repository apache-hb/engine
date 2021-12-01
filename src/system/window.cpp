#include "window.h"

#include <format>
#include "logging/log.h"
#include "util/win32.h"

namespace {
    LRESULT CALLBACK WindowHandleCallback(
        HWND hwnd,
        UINT msg,
        WPARAM wparam,
        LPARAM lparam
    )
    {
        auto *self = reinterpret_cast<engine::win32::WindowHandle*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
        LPCREATESTRUCT create;

        switch (msg) {
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

namespace engine::win32 {
    WindowHandle::WindowHandle(
        HINSTANCE instance,
        int show,
        LPCTSTR title,
        LONG width,
        LONG height
    ) : instance(instance), name(title) {
        logging::bug->check(instance != nullptr, Error("instance != nullptr"))
                     ->check(width >= 0, Error("width >= 0"))
                     ->check(height >= 0, Error("height >= 0"));

        WNDCLASSEX wc = {
            .cbSize = sizeof(WNDCLASSEX),
            .style = CS_HREDRAW | CS_VREDRAW,
            .lpfnWndProc = WindowHandleCallback,
            .hInstance = instance,
            .hCursor = LoadCursor(NULL, IDC_ARROW),
            .lpszClassName = name
        };

        logging::bug->check(
            RegisterClassEx(&wc) != 0, 
            win32::Win32Error("register-class")
        );

        RECT rect = {
            .right = width,
            .bottom = height
        };

        logging::bug->check(
            AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, false) != 0,
            win32::Win32Error("adjust-window-rect")
        );

        handle = CreateWindow(
            name, name, 
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, CW_USEDEFAULT,
            width, height,
            nullptr, nullptr,
            instance, this
        );

        logging::bug->check(
            handle != nullptr,
            win32::Win32Error("create-window")
        );

        logging::bug->check(
            ShowWindow(handle, show) == 0,
            win32::Win32Error("show-window")
        );
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
        logging::bug->check(
            GetClientRect(handle, &rect) == 0,
            win32::Win32Error("get-client-rect")
        );
        return rect;
    }

    WindowHandle::Size WindowHandle::getClientSize() const {
        RECT rect = getClientRect();
        return WindowHandle::Size(rect.right - rect.left, rect.bottom - rect.top);
    }
}
