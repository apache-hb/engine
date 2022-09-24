#include "engine/base/panic.h"
#include "engine/base/win32.h"

NORETURN engine::panic(const engine::PanicInfo &info, const std::string &msg) {
    MessageBox(nullptr, msg.c_str(), fmt::format("{}:{}@{}", info.file, info.line, info.function).c_str(), MB_ICONSTOP);
    std::abort();
}
