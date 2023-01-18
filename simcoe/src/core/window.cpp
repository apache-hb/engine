#include "simcoe/core/system.h"

using namespace simcoe;
using namespace simcoe::system;

namespace {
    constexpr const char *kpzClassName = "simcoe::Window";
    HMODULE khInstance = GetModuleHandle(nullptr);

    DWORD getStyle(WindowStyle style) {
        switch (style) {
        case WindowStyle::eWindow: return WS_OVERLAPPEDWINDOW;
        case WindowStyle::eBorderless: return WS_POPUP;
        default: return 0; // TODO: assert unreachable
        }
    }
}

Window::Window(const char *pzTitle, const Size& size, WindowStyle style) { 
    // TODO: validate args

    RECT desktop;
    GetClientRect(GetDesktopWindow(), &desktop);

    const WNDCLASSEX kClass = {
        .cbSize = sizeof(WNDCLASSEX),
        .style = CS_HREDRAW | CS_VREDRAW,
        .lpfnWndProc = Window::callback,
        .hInstance = khInstance,
        .hCursor = LoadCursor(nullptr, IDC_ARROW),
        .lpszClassName = kpzClassName
    };
    RegisterClassEx(&kClass);

    hWindow = CreateWindow(
        /* lpClassName = */ kpzClassName,
        /* lpWindowName = */ pzTitle,
        /* dwStyle = */ getStyle(style),
        /* x = */ int(desktop.right - size.width) / 2,
        /* y = */ int(desktop.bottom - size.height) / 2,
        /* nWidth = */ int(size.width),
        /* nHeight = */ int(size.height),
        /* hWndParent = */ nullptr,
        /* hMenu = */ nullptr,
        /* hInstance = */ khInstance,
        /* lpParam = */ this
    );

    ShowWindow(hWindow, SW_SHOW);
    UpdateWindow(hWindow);
}

Window::~Window() {
    DestroyWindow(hWindow);
    UnregisterClass(kpzClassName, khInstance);
}

void Window::restyle(WindowStyle style) {
    SetWindowLong(hWindow, GWL_STYLE, getStyle(style));
}

LRESULT CALLBACK Window::callback(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    Window *pWindow = reinterpret_cast<Window*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

    switch (msg) {
    case WM_CREATE: {
        auto *pCreateStruct = reinterpret_cast<CREATESTRUCT*>(lparam);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pCreateStruct->lpCreateParams));
        break;
    }

    case WM_SETFOCUS:
        pWindow->bHasFocus = true;
        break;
    case WM_KILLFOCUS:
        pWindow->bHasFocus = false;
        break;

    case WM_ACTIVATE:
        pWindow->bHasFocus = (LOWORD(wparam) != WA_INACTIVE);
        break;

    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcA(hwnd, msg, wparam, lparam);
}

bool Window::poll() {
    MSG msg { };
    while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE) != 0) {
        if (msg.message == WM_QUIT) {
            return false;
        }

        if (msg.message == WM_PAINT) {
            continue;
        }

        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    return true;
}

Size Window::size() {
    RECT rect;
    GetClientRect(hWindow, &rect);
    return Size { size_t(rect.right - rect.left), size_t(rect.bottom - rect.top) };
}
