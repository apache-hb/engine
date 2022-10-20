#include "engine/base/win32.h"

#include <stdlib.h>
#include <dbghelp.h>

using namespace simcoe;

namespace {
    constexpr size_t kLoopLimit = 64;
}

void win32::init() {
    // enable stackwalker
    SymInitialize(GetCurrentProcess(), nullptr, true);
    
    // we want to be dpi aware
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    
    // shut up abort
    _set_abort_behavior(0, _WRITE_ABORT_MSG);
}

void win32::showCursor(bool show) {
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
