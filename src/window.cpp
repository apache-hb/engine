#include "window.h"

namespace 
{
    HRESULT CALLBACK WindowHandleCallback(
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
            self->onCreate();
            return 0;

        case WM_DESTROY:
            self->onDestroy();
            return 0;

        case WM_QUIT:
            PostQuitMessage(0);
            return 0;
        }
        return DefWindowProc(hwnd, msg, wparam, lparam);
    }
}

WindowHandle::WindowHandle(
    HINSTANCE instance,
    int show,
    LPTSTR title,
    LONG width,
    LONG height
) : instance(instance)
  , name(title)
{
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

WindowHandle::~WindowHandle()
{
    DestroyWindow(handle);
    UnregisterClass(name, instance);
}

void WindowHandle::run()
{
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
