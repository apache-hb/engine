#include "window.h"

#include "utils.h"

namespace 
{
    LRESULT CALLBACK WindowHandleCallback(
        HWND hwnd,
        UINT msg,
        WPARAM wparam,
        LPARAM lparam
    )
    {
        WindowHandle *self = reinterpret_cast<WindowHandle*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
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

WindowHandle::WindowHandle(
    HINSTANCE instance,
    int show,
    LPCTSTR title,
    LONG width,
    LONG height
) : instance(instance), name(title)
{
    ASSERT(title != nullptr);
    ASSERT(width >= 0);
    ASSERT(height >= 0);

    WNDCLASSEX wc = {
        .cbSize = sizeof(WNDCLASSEX),
        .style = CS_HREDRAW | CS_VREDRAW,
        .lpfnWndProc = WindowHandleCallback,
        .hInstance = instance,
        .hCursor = LoadCursor(NULL, IDC_ARROW),
        .lpszClassName = name
    };

    RegisterClassEx(&wc);

    RECT rect = {
        .right = width,
        .bottom = height
    };

    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, false);

    handle = CreateWindow(
        name, name, 
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        width, height,
        nullptr, nullptr,
        instance, this
    );

    ShowWindow(handle, show);
}

WindowHandle::~WindowHandle() {
    DestroyWindow(handle);
    UnregisterClass(name, instance);
}

void WindowHandle::run() {
    onCreate();

    MSG msg = { };
    while (msg.message != WM_QUIT)
    {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
}

RECT WindowHandle::getClientRect() const {
    RECT rect;
    GetClientRect(handle, &rect);
    return rect;
}

WindowHandle::Size WindowHandle::getClientSize() const {
    RECT rect = getClientRect();
    return WindowHandle::Size(rect.right - rect.left, rect.bottom - rect.top);
}
