#include "system.h"

namespace {
    LRESULT CALLBACK WindowHandleCallback(
        HWND hwnd,
        UINT msg,
        WPARAM wparam,
        LPARAM lparam
    )
    {
        auto *self = reinterpret_cast<engine::system::Window::Callbacks*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

        switch (msg) {
        case WM_CREATE: {
            LPCREATESTRUCT create = reinterpret_cast<LPCREATESTRUCT>(lparam);
            SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(create->lpCreateParams));
            break;
        }

        case WM_KEYDOWN:
            self->onKeyPress(static_cast<int>(wparam));
            break;

        case WM_KEYUP:
            self->onKeyRelease(static_cast<int>(wparam));
            break;

        case WM_PAINT:
            self->repaint();
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

    win32::Result<Window> createWindow(HINSTANCE instance, const Window::Create &create, Window::Callbacks *callbacks) {
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
            .hCursor = LoadCursor(NULL, IDC_ARROW),
            .lpszClassName = name
        };

        if (RegisterClassEx(&wc) == 0) {
            return fail(GetLastError());
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
            return fail(GetLastError());
        }

        return pass(Window(instance, name, handle, callbacks));
    }

    void destroyWindow(Window &window) {
        DestroyWindow(window.getHandle());
        UnregisterClass(window.getName(), window.getInstance());
    }
}
