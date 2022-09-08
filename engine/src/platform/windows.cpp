#include "engine/platform/platform.h"

using namespace engine::platform;

namespace {
    constexpr auto kClassName = "EngineWindowClass";

    [[nodiscard]]
    std::string getComputerName() {
        constexpr auto kNameLength = MAX_COMPUTERNAME_LENGTH + 1;
        
        char name[kNameLength];
        DWORD size = kNameLength;

        if (!GetComputerNameA(name, &size)) {
            return "";
        }

        return std::string {name, size};
    }

    using MonitorList = std::vector<HMONITOR>;

    BOOL CALLBACK enumMonitorProc(HMONITOR monitor, HDC, LPRECT, LPARAM lparam) {
        auto *it = reinterpret_cast<MonitorList*>(lparam);
        it->push_back(monitor);
        return true;
    }

    LRESULT CALLBACK windowProc(HWND window, UINT msg, WPARAM wparam, LPARAM lparam) {
        // auto *self = reinterpret_cast<Window*>(GetWindowLongPtr(window, GWLP_USERDATA));

        switch (msg) {
        case WM_CREATE: {
            auto create = reinterpret_cast<LPCREATESTRUCT>(lparam);
            SetWindowLongPtr(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(create));
            break;
        }

        case WM_MOUSEMOVE:
        case WM_KEYDOWN: 
        case WM_KEYUP:
            break;

        case WM_DESTROY:
            PostQuitMessage(0);
            break;

        case WM_CLOSE:
            DestroyWindow(window);
            return 0;

        default:
           break;
        }
        return DefWindowProcA(window, msg, wparam, lparam);
    }
}

void Window::create(HINSTANCE instance, const char *title, Size size, Position position) {
    window = CreateWindowExA(
        /* dwExStyle = */ 0,
        /* lpClassName = */ kClassName,
        /* lpWindowName = */ title,
        /* dwStyle = */ WS_OVERLAPPEDWINDOW ^ (WS_THICKFRAME | WS_MAXIMIZEBOX),
        /* X = */ position.x,
        /* Y = */ position.y,
        /* nWidth = */ size.width,
        /* height = */ size.height,
        /* hWndParent = */ nullptr,
        /* hMenu = */ nullptr,
        /* hInstance = */ instance,
        /* lpParam = */ this
    );

    ShowWindow(window, SW_SHOW);
}

System::System(HINSTANCE instance)
    : name(getComputerName())
    , instance(instance)
{ 
    /* make sure we get the current monitor sizes */
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    /* collect all monitors */
    MonitorList monitors;
    EnumDisplayMonitors(nullptr, nullptr, enumMonitorProc, (LPARAM)&monitors);

    /* convert them to displays */
    for (size_t i = 0; i < monitors.size(); i++) {
        MONITORINFOEXA info;
        info.cbSize = sizeof(MONITORINFOEXA);
        GetMonitorInfoA(monitors[i], &info);

        auto [left, top, right, bottom] = info.rcMonitor;
        Size size = { right - left, bottom - top };
        Position position = { info.rcWork.left, info.rcWork.top };

        displays.push_back({ instance, size, position, info.szDevice });

        if (info.dwFlags & MONITORINFOF_PRIMARY) {
            primary = i; // TODO: assert there isnt already a primary display
        }
    }

    // TODO: assert primary display was found

    /* register our global window class */
    WNDCLASSEXA cls = { 
        .cbSize = sizeof(WNDCLASSEXA),
        .lpfnWndProc = windowProc,
        .hInstance = instance,
        .lpszClassName = kClassName
    };

    RegisterClassExA(&cls);
}

Display &System::primaryDisplay() {
    return displays[primary];
}

void System::run() {
    MSG msg = { };
    while (GetMessageA(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
}
