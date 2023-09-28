#include "simcoe/core/panic.h"
#include "simcoe/core/system.h"

#include <iostream>

using namespace simcoe;

void simcoe::panic(const PanicInfo& info, std::string_view msg) {
    auto it = std::format("[{}:{}@{}]: {}", info.file, info.fn, info.line, msg);
    std::cerr << it << std::endl;
    for (auto& trace : system::backtrace()) {
        std::cerr << trace << std::endl;
    }

    std::exit(1);
}
