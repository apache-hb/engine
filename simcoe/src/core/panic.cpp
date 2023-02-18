#include "simcoe/core/panic.h"
#include "simcoe/core/system.h"

using namespace simcoe;

NORETURN simcoe::panic(const simcoe::PanicInfo& info, const char *pzMessage) {
    printf("[%s:%s@%zu]: %s\n", info.pzFile, info.pzFunction, info.line, pzMessage);
    for (auto& trace : system::backtrace()) {
        printf("%s\n", trace.c_str());
    }

    std::exit(0);
}
