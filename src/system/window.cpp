#include "system.h"

#include "backends/imgui_impl_win32.h"

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
            return 0;
        }

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
            PostQuitMessage(0);
            return 0;

        default:
            return DefWindowProc(hwnd, msg, wparam, lparam);
        }
    }
}

namespace engine::system {
    void Window::popup(std::string_view title, std::string_view message) {
        MessageBoxA(handle, message.data(), title.data(), MB_ICONWARNING | MB_OK);
    }

    win32::Result<RECT> Window::getClientRect() const {
        RECT rect;
        if (!GetClientRect(handle, &rect)) {
            return fail(GetLastError());
        }
        return pass(rect);
    }

    win32::Result<Window::Size> Window::getClientSize() const {
        auto result = getClientRect();
        if (!result) { return result.forward(); }

        auto rect = result.value();
        auto width = rect.right - rect.left;
        auto height = rect.bottom - rect.top;

        return pass(Window::Size(width, height));
    }

    win32::Result<float> Window::getClientAspectRatio() const {
        auto result = getClientSize();
        if (!result) { return result.forward(); }

        auto [width, height] = result.value();
        return pass(float(width) / float(height));
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

    win32::Result<Window> createWindow(HINSTANCE instance, LPCTSTR name, Window::Size size, Window::Callbacks *callbacks) {
        WNDCLASSEX wc = {
            .cbSize = sizeof(WNDCLASSEX),
            .style = CS_HREDRAW | CS_VREDRAW,
            .lpfnWndProc = WindowHandleCallback,
            .hInstance = instance,
            .hCursor = LoadCursor(NULL, IDC_ARROW),
            .lpszClassName = name
        };

        auto [width, height] = size;
        RECT rect = { .right = width, .bottom = height };
        
        if (RegisterClassEx(&wc) == 0) {
            return fail(GetLastError());
        }

        if (AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, false) == 0) {
            return fail(GetLastError());
        }

        auto style = WS_OVERLAPPEDWINDOW; // WS_MAXIMIZE | WS_POPUP;

        HWND handle = CreateWindow(
            name, name, 
            style,
            CW_USEDEFAULT, CW_USEDEFAULT,
            width, height,
            nullptr, nullptr,
            instance, callbacks
        );

        if (handle == nullptr) {
            return fail(GetLastError());
        }

        return pass(Window(instance, name, handle, callbacks));
    }

    void destroyWindow(Window &window) {
        DestroyWindow(window.getHandle());
        UnregisterClass(window.getName(), window.getInstance());
    }
}
