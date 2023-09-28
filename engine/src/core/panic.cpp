#include "simcoe/core/panic.h"
#include "simcoe/core/system.h"

#include <iostream>

#include <dbghelp.h>

using namespace simcoe;

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

    void printBacktrace(std::ostream& pipe) {
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

            BOOL hasSym = SymGetSymFromAddr(hProcess, frame.AddrPC.Offset, &disp, pSymbol.get());
            if (hasSym == FALSE) {
                pipe << std::format("frame@[{:#016x}]", frame.AddrPC.Offset) << std::endl;
                continue;
            }

            DWORD len = UnDecorateSymbolName(pSymbol->Name, name, kSymbolSize, UNDNAME_COMPLETE);
            if (len == 0) {
                pipe << std::format("frame@[{:#016x}]", frame.AddrPC.Offset) << std::endl;
                continue;
            }

            pipe << std::format("frame@{}: {}", frame.AddrPC.Offset, name) << std::endl;
        }
    }
}

void simcoe::panic(const PanicInfo& info, std::string_view msg) {
    auto it = std::format("[{}:{}@{}]: {}", info.file, info.fn, info.line, msg);
    std::cerr << it << std::endl;
    printBacktrace(std::cerr);
    std::abort();
}
