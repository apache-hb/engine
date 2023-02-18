#include "simcoe/core/system.h"

#include <mutex>

#include <dbghelp.h>
#include <objbase.h>
#include <stdlib.h>
#include <comdef.h>

using namespace simcoe;

namespace {
    std::mutex critical;
    constexpr size_t kSymbolSize = MAX_SYM_NAME;
    constexpr size_t kLoopLimit = 64;

    auto newSymbol() {
        auto release = [](IMAGEHLP_SYMBOL *pSymbol) {
            free(pSymbol);
        };
        return std::unique_ptr<IMAGEHLP_SYMBOL, decltype(release)>( 
            (IMAGEHLP_SYMBOL*)malloc(sizeof(IMAGEHLP_SYMBOL) + kSymbolSize),
            release
        );
    }
}

system::System::System() {
    // enable stackwalker
    SymInitialize(GetCurrentProcess(), nullptr, true);
    
    // we want to be dpi aware
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    
    // shut up abort
    _set_abort_behavior(0, _WRITE_ABORT_MSG);

    // enable xaudio
    HR_CHECK(CoInitializeEx(nullptr, COINIT_MULTITHREADED));
}

system::System::~System() {
    CoUninitialize();
    SymCleanup(GetCurrentProcess());
}

system::StackTrace system::backtrace() {
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

    while (getFrame(&frame, &ctx)) {
        pSymbol->SizeOfStruct = sizeof(IMAGEHLP_SYMBOL);
        pSymbol->MaxNameLength = kSymbolSize;

        char name[kSymbolSize] = { 0 };

        SymGetSymFromAddr(hProcess, frame.AddrPC.Offset, &disp, pSymbol.get());
        UnDecorateSymbolName(pSymbol->Name, name, kSymbolSize, UNDNAME_COMPLETE);

        co_yield name;
    }
}

void system::showCursor(bool show) {
    // ShowCursor maintans an internal counter, so we need to call it repeatedly

    // we *also* need a loop limit because ShowCursor returns -1 forever
    // when no mouse is connected, dont want to hang a thread
    size_t limit = 0;
    if (show) {
        while (ShowCursor(true) < 0 && limit++ < kLoopLimit);
    } else {
        while (ShowCursor(false) >= 0 && limit++ < kLoopLimit);
    }
}

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
