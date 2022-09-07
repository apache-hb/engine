#include "win32.h"

namespace engine::win32 {
    std::string toString(DWORD err) {
        auto flags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM;
        auto lang = MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT);
        HLOCAL buffer = nullptr;
        auto len = FormatMessage(flags, nullptr, err, lang, LPTSTR(&buffer), 0, nullptr);
    
        if (len == 0) {
            return std::to_string(err);
        }

        auto msg = LPCSTR(buffer);
        auto result = std::string(msg, msg + len);
        LocalFree(buffer);
        return result;
    }
}
