#include "simcoe/core/util.h"
#include "simcoe/core/win32.h"

using namespace simcoe;
using namespace simcoe::util;

std::string util::narrow(std::wstring_view wstr) {
    std::string result(wstr.size() + 1, '\0');
    size_t size = result.size();

    errno_t err = wcstombs_s(&size, result.data(), result.size(), wstr.data(), wstr.size());
    if (err != 0) {
        return "";
    }

    result.resize(size - 1);
    return result;
}

std::wstring util::widen(std::string_view str) {
    auto needed = MultiByteToWideChar(CP_UTF8, 0, str.data(), (int)str.size(), nullptr, 0);
    std::wstring result(needed, '\0');
    MultiByteToWideChar(CP_UTF8, 0, str.data(), (int)str.size(), result.data(), (int)result.size());
    return result;
}

std::string util::join(std::string_view sep, std::span<const std::string> parts) {
    std::string result;
    for (auto& part : parts) {
        if (!result.empty()) {
            result += sep;
        }
        result += part;
    }
    return result;
}

Entry::Entry(RegistryBase& base) 
    : base(base)
{
    std::lock_guard lock(base.mutex);
    base.entries.insert(this);
}

Entry::~Entry() {
    std::lock_guard lock(base.mutex);
    base.entries.erase(this);
}
