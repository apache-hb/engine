#include "system.h"

#include <windowsx.h>
#include "logging/log.h"
#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_win32.h"
#include "imgui/backends/imgui_impl_dx12.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace {
    LRESULT CALLBACK WindowHandleCallback(
        HWND hwnd,
        UINT msg,
        WPARAM wparam,
        LPARAM lparam
    )
    {
        if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam))
            return true;

        auto *self = reinterpret_cast<engine::system::Window::Callbacks*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

        switch (msg) {
        case WM_CREATE: {
            LPCREATESTRUCT create = reinterpret_cast<LPCREATESTRUCT>(lparam);
            SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(create->lpCreateParams));
            break;
        }

        case WM_KEYDOWN: case WM_KEYUP:
        case WM_MOUSEMOVE:
            self->addEvent({ msg, wparam, lparam });
            break;

        case WM_DESTROY:
            PostQuitMessage(0);
            break;

        case WM_CLOSE:
            if (self->onClose()) {
                DestroyWindow(hwnd);
            }
            return 0;
            
        default:
            break;
        }

        return DefWindowProc(hwnd, msg, wparam, lparam);
    }

    using WindowStyle = engine::system::Window::Style;

    DWORD selectStyle(WindowStyle style) {
        switch (style) {
        case WindowStyle::BORDERLESS:
            return WS_POPUP;

        case WindowStyle::WINDOWED:
            return WS_OVERLAPPEDWINDOW ^ (WS_THICKFRAME | WS_MAXIMIZEBOX);

        default:
            return WS_OVERLAPPEDWINDOW;
        }
    }
}

namespace engine::system {
    void Window::popup(std::string_view title, std::string_view message) {
        if (!MessageBoxA(handle, message.data(), title.data(), MB_ICONWARNING | MB_OK)) {
            log::global->warn("failed to show message box");
        }
    }

    RECT Window::getClientRect() const {
        RECT rect;
        if (!GetClientRect(handle, &rect)) {
            throw win32::Error("GetClientRect");
        }
        return rect;
    }

    Window::Size Window::getClientSize() const {
        auto rect = getClientRect();
        auto width = rect.right - rect.left;
        auto height = rect.bottom - rect.top;

        return Window::Size(width, height);
    }

    float Window::getClientAspectRatio() const {
        auto [width, height] = getClientSize();
        return float(width) / float(height);
    }

    size_t Window::getDpi() const {
        return GetDpiForWindow(handle);
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

    Window createWindow(HINSTANCE instance, const Window::Create &create, Window::Callbacks *callbacks) {
        auto name = create.title;
        auto rect = create.rect;
        auto x = rect.left;
        auto y = rect.top;
        auto width = rect.right - rect.left;
        auto height = rect.bottom - rect.top;

        WNDCLASSEX wc = {
            .cbSize = sizeof(WNDCLASSEX),
            .style = CS_HREDRAW | CS_VREDRAW,
            .lpfnWndProc = WindowHandleCallback,
            .hInstance = instance,
            .hCursor = LoadCursor(nullptr, IDC_ARROW),
            .lpszClassName = name
        };

        if (RegisterClassEx(&wc) == 0) {
            throw win32::Error("RegisterClassEx");
        }

        // disable resizing and maximizing
        auto style = selectStyle(create.style);

        HWND handle = CreateWindow(
            name, name,
            style,
            x, y,
            width, height,
            nullptr, nullptr,
            instance, callbacks
        );

        if (handle == nullptr) {
            throw win32::Error("CreateWindow");
        }

        return Window(instance, name, handle, callbacks);
    }

    void destroyWindow(Window &window) {
        DestroyWindow(window.getHandle());
        UnregisterClass(window.getName(), window.getInstance());
    }
}
