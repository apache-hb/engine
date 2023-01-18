#include "simcoe/core/system.h"

#include <mutex>

#include <windows.h>
#include <dbghelp.h>
#include <objbase.h>
#include <stdlib.h>
#include <comdef.h>

using namespace simcoe;

namespace {
    std::mutex critical;
    constexpr size_t kSymbolSize = MAX_SYM_NAME;
}

void system::init() {
    // enable stackwalker
    SymInitialize(GetCurrentProcess(), nullptr, true);
    
    // we want to be dpi aware
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    
    // shut up abort
    _set_abort_behavior(0, _WRITE_ABORT_MSG);
}

void system::deinit() {
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

    IMAGEHLP_SYMBOL *pSymbol = (IMAGEHLP_SYMBOL*)malloc(sizeof(IMAGEHLP_SYMBOL) + kSymbolSize);
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

        SymGetSymFromAddr(hProcess, frame.AddrPC.Offset, &disp, pSymbol);
        UnDecorateSymbolName(pSymbol->Name, name, kSymbolSize, UNDNAME_COMPLETE);

        co_yield name;
    }
}
