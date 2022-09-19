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
        auto *self = reinterpret_cast<Window*>(GetWindowLongPtrA(window, GWLP_USERDATA));

        switch (msg) {
        case WM_CREATE: {
            auto create = reinterpret_cast<LPCREATESTRUCT>(lparam);
            SetWindowLongPtrA(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(create->lpCreateParams));
            break;
        }

        case WM_MOUSEMOVE:
        case WM_KEYDOWN: 
        case WM_KEYUP:
            self->addEvent({ msg, wparam, lparam });
            break;

        case WM_DESTROY:
        case WM_QUIT:
            self->close();
            self->addEvent({ msg, wparam, lparam }); // add the close event to make the main loop work
            PostQuitMessage(0);
            break;

        case WM_CLOSE:
            DestroyWindow(window);
            break;

        default:
           break;
        }
        return DefWindowProcA(window, msg, wparam, lparam);
    }
}

void Window::addEvent(Event event) { 
    events.push(event); 
}

void Window::close() {
    running = false;
}

std::optional<Event> Window::getEvent() {
    if (!running) {
        return std::nullopt;
    }
    return events.pop();
}

void Window::create(HINSTANCE instance, const char *title, Size size, Position position) {
    window = CreateWindowA(
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

Window *Display::open(const std::string &title, Size windowSize, Position windowPos) {
    if (windowPos == kCenter) {
        windowPos = Position::from((size.width - windowSize.width) / 2, (size.height - windowSize.height) / 2);
    }
    
    auto *it = new Window();
    it->create(instance, title.c_str(), windowSize, windowPos + position);
    return it;
}

System::System(HINSTANCE instance)
    : name(getComputerName())
    , primary(SIZE_MAX)
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
        Size size { right - left, bottom - top };
        Position position { info.rcWork.left, info.rcWork.top };

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
    MSG msg { };
    while (GetMessageA(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
}
