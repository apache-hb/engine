#include "simcoe/core/system.h"

#include "simcoe/simcoe.h"

#include <mutex>

#include <dbghelp.h>
#include <objbase.h>
#include <stdlib.h>
#include <comdef.h>

using namespace simcoe;
using namespace simcoe::system;

///
/// window object
///

namespace {
    constexpr auto kWindowClassName = "simcoe";

    Window *getWindowPtr(HWND hwnd) {
        return reinterpret_cast<Window*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }

    DWORD getStyle(WindowStyle style) {
        switch (style) {
        case WindowStyle::eWindow: return WS_OVERLAPPEDWINDOW;
        case WindowStyle::eBorderless: return WS_POPUP;
        default: NEVER("invalid window style");
        }
    }
}

// callback

LRESULT CALLBACK Window::callback(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    Window *pWindow = getWindowPtr(hwnd);

    switch (msg) {
    case WM_DESTROY: {
        if (pWindow == nullptr) break;
        IWindowEvents *events = pWindow->getEvents();

        events->onClose();
        return 0;
    }

    case WM_CREATE: {
        auto *pCreateStruct = reinterpret_cast<CREATESTRUCT*>(lParam);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pCreateStruct->lpCreateParams));

        Window *pIncomingWindow = getWindowPtr(hwnd);
        IWindowEvents *events = pIncomingWindow->getEvents();

        events->onOpen();
        break;
    }

    case WM_ACTIVATE: {
        if (pWindow == nullptr) break;
        IWindowEvents *events = pWindow->getEvents();

        switch (LOWORD(wParam)) {
        case WA_ACTIVE:
        case WA_CLICKACTIVE:
            events->onGainFocus();
            break;

        default:
            events->onLoseFocus();
            break;
        }

        return 0;
    }

    default:
        break;
    }

    if (pWindow != nullptr) {
        IWindowEvents *events = pWindow->getEvents();
        if (events->onEvent(hwnd, msg, wParam, lParam)) {
            return 0;
        }
    }

    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

Window::Window(HINSTANCE hInstance, int nCmdShow, const WindowInfo& info) {
    events = info.pEvents;

    auto [width, height] = info.size;

    int x = (GetSystemMetrics(SM_CXSCREEN) - int(width)) / 2;
    int y = (GetSystemMetrics(SM_CYSCREEN) - int(height)) / 2;

    const WNDCLASSEX kClass = {
        .cbSize = sizeof(WNDCLASSEX),
        .style = CS_HREDRAW | CS_VREDRAW,
        .lpfnWndProc = Window::callback,
        .hInstance = hInstance,
        .hCursor = NULL,
        .lpszClassName = kWindowClassName
    };
    RegisterClassEx(&kClass);

    hWindow = CreateWindow(
        /* lpClassName = */ kWindowClassName,
        /* lpWindowName = */ info.title.data(),
        /* dwStyle = */ getStyle(info.style),
        /* x = */ x,
        /* y = */ y,
        /* nWidth = */ int(width),
        /* nHeight = */ int(height),
        /* hWndParent = */ nullptr,
        /* hMenu = */ nullptr,
        /* hInstance = */ hInstance,
        /* lpParam = */ this
    );

    ShowWindow(hWindow, nCmdShow);
    UpdateWindow(hWindow);
}

Window::~Window() {
    DestroyWindow(hWindow);
}

// window manipulation

void Window::restyle(WindowStyle style) {
    SetWindowLong(hWindow, GWL_STYLE, getStyle(style));
}

void Window::hideCursor(bool hide) {
    bHideCursor = hide;
    if (hide) {
        SetCursor(nullptr);
    } else {
        SetCursor(LoadCursor(nullptr, IDC_ARROW));
    }
}

// window queries

float Window::getDpi() const {
    return float(GetDpiForWindow(hWindow));
}

Size Window::getSize() const {
    RECT rect;
    GetClientRect(hWindow, &rect);

    Size size = {
        .width = size_t(rect.right - rect.left),
        .height = size_t(rect.bottom - rect.top)
    };

    gRenderLog.info("window size: {} x {}", size.width, size.height);

    return size;
}

HWND Window::getHandle() const {
    return hWindow;
}

IWindowEvents* Window::getEvents() const {
    return events;
}

///
/// system object
///

System::System(HINSTANCE hInstance) : hInstance(hInstance) {
    const WNDCLASSA cls = {
        .style = CS_HREDRAW | CS_VREDRAW,
        .lpfnWndProc = Window::callback,
        .hInstance = hInstance,
        .hIcon = nullptr,
        .hCursor = nullptr,
        .hbrBackground = nullptr,
        .lpszMenuName = nullptr,
        .lpszClassName = kWindowClassName,
    };

    if (!RegisterClassA(&cls)) {
        PANIC("failed to register window class");
    }

    // enable stackwalker
    SymInitialize(GetCurrentProcess(), nullptr, true);

    // we want to be dpi aware
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_SYSTEM_AWARE);

    // shut up abort
    _set_abort_behavior(0, _WRITE_ABORT_MSG);

    // enable xaudio
    HR_CHECK(CoInitializeEx(nullptr, COINIT_MULTITHREADED));
}

System::~System() {
    UnregisterClass(kWindowClassName, hInstance);

    CoUninitialize();
    SymCleanup(GetCurrentProcess());
}

Window System::openWindow(int nCmdShow, const WindowInfo& info) {
    return Window(hInstance, nCmdShow, info);
}

bool System::poll() {
    MSG msg = { };
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

///
/// timer object
///

namespace {
    std::mutex critical;
    constexpr size_t kSymbolSize = MAX_SYM_NAME;

    auto newSymbol() {
        auto release = [](IMAGEHLP_SYMBOL *pSymbol) {
            free(pSymbol);
        };

        return std::unique_ptr<IMAGEHLP_SYMBOL, decltype(release)>(
            (IMAGEHLP_SYMBOL*)malloc(sizeof(IMAGEHLP_SYMBOL) + kSymbolSize),
            release
        );
    }

    size_t getPerfCounter() {
        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);
        return size_t(now.QuadPart);
    }

    size_t getPerfFrequency() {
        LARGE_INTEGER freq;
        QueryPerformanceFrequency(&freq);

        return size_t(freq.QuadPart);
    }

    auto kFrequency = getPerfFrequency();
}

Timer::Timer()
    : last(getPerfCounter())
{ }

float Timer::tick() {
    size_t now = getPerfCounter();
    float elapsed = float(now - last) / kFrequency;
    last = now;
    return elapsed;
}

///
/// error formatting
///

std::string system::hrString(HRESULT hr) {
    _com_error err(hr);
    return std::format("{} (0x{:x})", err.ErrorMessage(), unsigned(hr));
}

std::string system::win32String(DWORD dw) {
    LPSTR messageBuffer = nullptr;
    size_t size = FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr,
        dw,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPSTR)&messageBuffer,
        0,
        nullptr
    );

    std::string message(messageBuffer, size);

    while (message.ends_with("\r") || message.ends_with("\n")) {
        message.pop_back();
    }

    LocalFree(messageBuffer);

    return message;
}

std::string system::win32LastErrorString() {
    return win32String(GetLastError());
}

///
/// backtrace
///

std::vector<std::string> system::backtrace() {
    HANDLE hProcess = GetCurrentProcess();
    HANDLE hThread = GetCurrentThread();

    auto getFrame = [hProcess, hThread](STACKFRAME *pFrame, CONTEXT *pContext) {
        return StackWalk(
            /* MachineType = */ IMAGE_FILE_MACHINE_AMD64,
            /* hProcess = */ hProcess,
            /* hThread = */ hThread,
            /* StackFrame = */ pFrame,
            /* Context = */ pContext,
            /* ReadMemoryRoutine = */ nullptr,
            /* FunctionTableAccessRoutine = */ SymFunctionTableAccess64,
            /* GetModuleBaseRoutine = */ SymGetModuleBase64,
            /* TranslateAddress = */ nullptr
        );
    };

    auto pSymbol = newSymbol();

    pSymbol->SizeOfStruct = sizeof(IMAGEHLP_SYMBOL);
    pSymbol->Address = 0;
    pSymbol->Size = 0;
    pSymbol->Flags = SYMF_FUNCTION;
    pSymbol->MaxNameLength = kSymbolSize;

    DWORD64 disp = 0;

    std::lock_guard guard(critical);

    CONTEXT ctx = { };
    RtlCaptureContext(&ctx);

    STACKFRAME frame = {
        .AddrPC = {
            .Offset = ctx.Rip,
            .Mode = AddrModeFlat
        },
        .AddrFrame = {
            .Offset = ctx.Rbp,
            .Mode = AddrModeFlat
        },
        .AddrStack = {
            .Offset = ctx.Rsp,
            .Mode = AddrModeFlat
        }
    };

    std::vector<std::string> result;

    while (getFrame(&frame, &ctx)) {
        pSymbol->SizeOfStruct = sizeof(IMAGEHLP_SYMBOL);
        pSymbol->MaxNameLength = kSymbolSize;

        char name[kSymbolSize] = { 0 };

        BOOL hasSym = SymGetSymFromAddr(hProcess, frame.AddrPC.Offset, &disp, pSymbol.get());
        if (hasSym == FALSE) {
            result.push_back(std::format("frame@[{:#016x}]", frame.AddrPC.Offset));
            continue;
        }

        DWORD len = UnDecorateSymbolName(pSymbol->Name, name, kSymbolSize, UNDNAME_COMPLETE);
        if (len == 0) {
            result.push_back(std::format("frame@[{:#016x}]", frame.AddrPC.Offset));
            continue;
        }

        result.push_back(name);
    }

    return result;
}
