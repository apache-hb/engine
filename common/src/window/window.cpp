#include "engine/window/window.h"

#include "engine/base/logging.h"

#include <windowsx.h>
#include <winuser.h>

#include "imgui.h"
#include "imgui_impl_win32.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

using namespace simcoe;

namespace {
    constexpr auto kClassName = "GameWindowClass";
    auto instance = GetModuleHandleA(nullptr);

    // handling keyboard accuratley is more tricky than it first seems
    // https://learn.microsoft.com/en-us/windows/win32/inputdev/about-keyboard-input#keystroke-message-flags
    WORD mapKeyCode(WPARAM wparam, LPARAM lparam) {
        WORD vkCode = LOWORD(wparam);
        WORD keyFlags = HIWORD(lparam);
        WORD scanCode = LOBYTE(keyFlags);

        if ((keyFlags & KF_EXTENDED) == KF_EXTENDED) {
            scanCode = MAKEWORD(scanCode, 0xE0);
        }

        if (vkCode == VK_SHIFT || vkCode == VK_CONTROL || vkCode == VK_MENU) {
            vkCode = LOWORD(MapVirtualKeyA(scanCode, MAPVK_VSC_TO_VK_EX));
        }

        return vkCode;
    }

    LRESULT CALLBACK windowCallback(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
        Window *pWindow = reinterpret_cast<Window*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

        switch (msg) {
        case WM_CREATE: {
            auto *pCreateStruct = reinterpret_cast<CREATESTRUCT*>(lparam);
            SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pCreateStruct->lpCreateParams));
            break;
        }

        case WM_KEYDOWN:
            pWindow->keys[mapKeyCode(wparam, lparam)] = pWindow->index++;
            break;
        case WM_KEYUP:
            pWindow->keys[mapKeyCode(wparam, lparam)] = 0;
            break;
        case WM_SYSKEYDOWN:
            pWindow->keys[mapKeyCode(wparam, lparam)] = pWindow->index++;
            break;
        case WM_SYSKEYUP: 
            pWindow->keys[mapKeyCode(wparam, lparam)] = 0;
            break;

        case WM_RBUTTONDOWN:
            pWindow->keys[VK_RBUTTON] = pWindow->index++;
            break;
        case WM_RBUTTONUP:
            pWindow->keys[VK_RBUTTON] = 0;
            break;

        case WM_LBUTTONDOWN:
            pWindow->keys[VK_LBUTTON] = pWindow->index++;
            break;
        case WM_LBUTTONUP:
            pWindow->keys[VK_LBUTTON] = 0;
            break;

        case WM_MBUTTONDOWN:
            pWindow->keys[VK_MBUTTON] = pWindow->index++;
            break;
        case WM_MBUTTONUP:
            pWindow->keys[VK_MBUTTON] = 0;
            break;

        case WM_CLOSE:
            DestroyWindow(hwnd);
            return 0;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        }

        if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam)) {
            return true;
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

    pKeyboard->update(hwnd, keys);

    return true;
}
