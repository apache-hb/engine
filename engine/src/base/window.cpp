#include "engine/base/window.h"
#include "GLFW/glfw3.h"

#include <fmt/printf.h>

using namespace engine;

namespace {
    constexpr auto kClassName = "GameWindowClass";
    auto instance = GetModuleHandleA(nullptr);

    LRESULT CALLBACK windowCallback(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
        switch (msg) {
        case WM_CLOSE:
            DestroyWindow(hwnd);
            return 0;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        }
        return DefWindowProcA(hwnd, msg, wparam, lparam);
    }
}

Window::Window(int width, int height, const char *title) { 
    const WNDCLASSEXA kCls {
        .cbSize = sizeof(WNDCLASSEXA),
        .style = CS_HREDRAW | CS_VREDRAW,
        .lpfnWndProc = windowCallback,
        .hInstance = instance,
        .hCursor = LoadCursorA(nullptr, IDC_ARROW),
        .lpszClassName = kClassName
    };
    RegisterClassExA(&kCls);

    hwnd = CreateWindowA(
        /* lpClassName = */ kClassName,
        /* lpWindowName = */ title,
        /* dxStyle = */ WS_POPUP,
        /* x = */ 0,
        /* y = */ 0,
        /* nWidth = */ width,
        /* nHeight = */ height,
        /* hWndParent = */ nullptr,
        /* hMenu = */ nullptr,
        /* hInstance = */ instance,
        /* lpParam = */ nullptr
    );

    ShowWindow(hwnd, SW_SHOW);
}

Window::~Window() {
    DestroyWindow(hwnd);
    UnregisterClassA(kClassName, instance);
}

math::Resolution<int> Window::size() const {
    RECT client;
    GetClientRect(hwnd, &client);
    return { client.right - client.left, client.bottom - client.top };
}

HWND Window::handle() {
    return hwnd;
}

bool Window::poll() {
    MSG msg { };
    if (GetMessageA(&msg, nullptr, 0, 0) != 0) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
        return true;
    }
    return false;
}
