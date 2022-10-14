#include "engine/base/panic.h"
#include "engine/base/win32.h"
#include "engine/base/util.h"

#include <mutex>
#include <vector>

#include <dbghelp.h>

namespace {
    std::mutex critical;

    constexpr size_t kSymbolSize = 256;

    auto hProcess = GetCurrentProcess();
    
    bool getFrame(STACKFRAME *pFrame, CONTEXT *pContext, HANDLE hThread) {
        return StackWalk(
            /* MachineType = */ IMAGE_FILE_MACHINE_AMD64,
            /* hProcess = */ hProcess,
            /* kThread = */ hThread,
            /* StackFrame = */ pFrame,
            /* Context = */ pContext,
            /* ReadMemoryRoutine = */ nullptr,
            /* FunctionTableAccessRoutine = */ SymFunctionTableAccess64,
            /* GetModuleBaseRoutine = */ SymGetModuleBase64,
            /* TranslateAddress = */ nullptr
        );
    }

    std::vector<std::string> getStackTrace() {
        auto hThread = GetCurrentThread();
    
        auto *pSymbol = (IMAGEHLP_SYMBOL*)malloc(sizeof(IMAGEHLP_SYMBOL) + kSymbolSize);
        pSymbol->SizeOfStruct = sizeof(IMAGEHLP_SYMBOL);
        pSymbol->Address = 0;
        pSymbol->Size = 0;
        pSymbol->Flags = SYMF_FUNCTION;
        pSymbol->MaxNameLength = kSymbolSize;

        DWORD64 disp = 0;

        std::vector<std::string> names;

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

        while (getFrame(&frame, &ctx, hThread)) {
            char name[kSymbolSize] = {};

            SymGetSymFromAddr(hProcess, frame.AddrPC.Offset, &disp, pSymbol);
            UnDecorateSymbolName(pSymbol->Name, name, kSymbolSize, UNDNAME_COMPLETE);
        
            names.push_back(std::format("{}", pSymbol->Name));
        };

        return names;
    }
}

NORETURN simcoe::panic(const simcoe::PanicInfo &info, const std::string &msg) {
    ShowCursor(true);
    auto stack = getStackTrace();
    MessageBox(nullptr, simcoe::strings::join<std::string>(stack, "\n").c_str(), std::format("error: {} at [{}:{}@{}]", msg, info.file, info.line, info.function).c_str(), MB_ICONSTOP);
    std::abort();
}
