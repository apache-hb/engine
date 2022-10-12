#include "engine/window/window.h"

#include "engine/base/logging.h"

#include <windowsx.h>

#include "imgui.h"
#include "imgui_impl_win32.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

using namespace engine;

namespace {
    constexpr auto kClassName = "GameWindowClass";
    auto instance = GetModuleHandleA(nullptr);

    Window::Info *getUser(HWND hwnd) {
        auto *pInfo = reinterpret_cast<Window::Info*>(GetWindowLongPtrA(hwnd, GWLP_USERDATA));
        pInfo->dirty = true;
        return pInfo;
    }

    constexpr Window::Key getKey(WPARAM wparam) {
        switch (wparam) {
        case 'W': return Window::eW;
        case 'A': return Window::eA;
        case 'S': return Window::eS;
        case 'D': return Window::eD;
        case 'E': return Window::eE;
        case 'Q': return Window::eQ;

        default: return Window::eUnknown;
        }
    }

    constexpr float getMovementWithPriority(size_t neg, size_t pos) {
        if (neg > 0 || pos > 0) {
            return (neg > pos) ? -1.f : 1.f;
        } else {
            return 0.f;
        }
    }

    LRESULT CALLBACK windowCallback(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
        if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam)) {
            return true;
        }

        switch (msg) {
        case WM_CREATE: {
            auto *pCreate = reinterpret_cast<LPCREATESTRUCTA>(lparam);
            auto *pInfo = reinterpret_cast<Window::Info*>(pCreate->lpCreateParams);
            SetWindowLongPtrA(hwnd, GWLP_USERDATA, LONG_PTR(pInfo));
            break;
        }

        case WM_KEYUP: {
            auto *pUser = getUser(hwnd);
            pUser->pressed[getKey(wparam)] = 0;
            break;
        }

        case WM_KEYDOWN: {
            auto *pUser = getUser(hwnd);
            if (wparam == VK_OEM_3) {
                pUser->toggleConsole = !pUser->toggleConsole;
            }
            pUser->pressed[getKey(wparam)] = ++pUser->priority;
            break;
        }

        case WM_MOUSEMOVE: {
            auto *pUser = getUser(hwnd);
            auto x = GET_X_LPARAM(lparam);
            auto y = GET_Y_LPARAM(lparam);

            pUser->mousePosition = { x, y };

            break;
        }

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
        /* lpParam = */ &info
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

void Window::imguiNewFrame() {
    ImGui_ImplWin32_NewFrame();
}

math::Vec2<int> Window::center() {
    RECT client;
    GetClientRect(hwnd, &client);
    return { (client.right - client.left) / 2, (client.bottom - client.top) / 2 };
}

bool Window::poll(input::Input *input) {
    ShowCursor(input->enableConsole);
    
    MSG msg { };
    while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE) != 0) {
        if (msg.message == WM_QUIT) {
            return false;
        }

        if (msg.message != WM_PAINT) { 
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }
    }

    auto [x, y] = center();

    if (!input->enableConsole) {
        SetCursorPos(x, y);
    }
    
    auto delta = lastMouseEvent.tick();

    // TODO: we could probably extract out the isDirty logic
    if (info.dirty) {
        info.dirty = false;
        input->device = input::eDesktop;

        // TODO: ugly way of doing defaults
        if (info.toggleConsole != SIZE_MAX) {
            input->enableConsole = info.toggleConsole;
        }

        input->movement = {
            .x = getMovementWithPriority(info.pressed[Window::eA], info.pressed[Window::eD]),
            .y = getMovementWithPriority(info.pressed[Window::eS], info.pressed[Window::eW]),
            .z = getMovementWithPriority(info.pressed[Window::eQ], info.pressed[Window::eE])
        };

        auto [mX, mY] = info.mousePosition;

        // TODO: multiplying by a random number is stupid, there has to be a better fix right?
        input->rotation = {
            .x = 200 * (mX - x) * float(delta),
            .y = 200 * (y - mY) * float(delta)
        };
    }

    return true;
}
