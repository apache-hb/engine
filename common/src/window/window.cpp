#include "engine/window/window.h"

#include "engine/base/logging.h"

#include <windowsx.h>

#include "imgui.h"
#include "imgui_impl_win32.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

using namespace simcoe;

namespace {
    constexpr auto kClassName = "GameWindowClass";
    auto instance = GetModuleHandleA(nullptr);

    LRESULT CALLBACK windowCallback(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
        ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam);

        Window *pWindow = reinterpret_cast<Window*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

        switch (msg) {
        case WM_CREATE: {
            auto *pCreateStruct = reinterpret_cast<CREATESTRUCT*>(lparam);
            SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pCreateStruct->lpCreateParams));
            break;
        }

        case WM_KEYDOWN:
            pWindow->keys[wparam] = true;
            break;
        case WM_KEYUP:
            pWindow->keys[wparam] = false;
            break;
        case WM_SYSKEYDOWN:
            pWindow->keys[wparam] = true;
            break;
        case WM_SYSKEYUP: 
            pWindow->keys[wparam] = false;
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
        /* lpParam = */ this
    );

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    ImGui_ImplWin32_Init(hwnd);
}

Window::~Window() {
    ImGui_ImplWin32_Shutdown();
    DestroyWindow(hwnd);
    UnregisterClassA(kClassName, instance);
}

math::Resolution<int> Window::size() const {
    RECT client;
    GetClientRect(hwnd, &client);
    return { client.right - client.left, client.bottom - client.top };
}

size_t Window::dpi() const {
    return GetDpiForWindow(hwnd);
}

HWND Window::handle() {
    return hwnd;
}

void Window::imgui() {
    ImGui_ImplWin32_NewFrame();
}

math::Vec2<int> Window::center() {
    RECT client;
    GetClientRect(hwnd, &client);
    return { (client.right - client.left) / 2, (client.bottom - client.top) / 2 };
}

bool Window::poll(input::Keyboard *pKeyboard) {
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

    pKeyboard->update(keys);

    return true;
}
