#include "simcoe/core/panic.h"
#include "simcoe/core/system.h"

using namespace simcoe;

NORETURN simcoe::panic(const simcoe::PanicInfo& info, const char *pzMessage) {
    printf("[%s:%s@%zu]: %s\n", info.pzFile, info.pzFunction, info.line, pzMessage);
    for (const char *pTrace : system::backtrace()) {
        printf("%s\n", pTrace);
    }

    std::exit(0);
}
