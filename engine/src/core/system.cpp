#include "simcoe/core/system.h"

#include <mutex>

#include <dbghelp.h>
#include <objbase.h>
#include <stdlib.h>
#include <comdef.h>

using namespace simcoe;
using namespace simcoe::system;

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

System::System() {
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
    CoUninitialize();
    SymCleanup(GetCurrentProcess());
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
