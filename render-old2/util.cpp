#include "util.h"

#include "util/win32.h"

#include <comdef.h>

extern "C" {
    DLLEXPORT extern DWORD NvOptimusEnablement = 0x00000001;
    DLLEXPORT extern int AmdPowerXpressRequestHighPerformance = 1;
}

namespace engine::render {
    std::string toString(HRESULT hr) {
        return strings::narrow(_com_error(hr).ErrorMessage());
    }

    std::string Error::query() const {
        return std::format("hresult(0x{:x}): {}\n{}", code & ULONG_MAX, toString(code), what());
    }

    void check(HRESULT hr, std::string_view message, std::source_location location) {
        if (FAILED(hr)) { throw render::Error(hr, std::string(message), location); }
    }
    
    void check(HRESULT hr, std::wstring_view message, std::source_location location) {
        return check(hr, strings::narrow(message), location);
    }
}
