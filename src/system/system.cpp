#include "system.h"

namespace engine::system {
    win32::Result<Stats> getStats() {
        std::string name(MAX_COMPUTERNAME_LENGTH + 1, ' ');
        DWORD size = MAX_COMPUTERNAME_LENGTH + 1;

        if (GetComputerNameA(name.data(), &size)) {
            name.resize(size);
            return pass(Stats{ name });
        }

        return fail(GetLastError());
    }
}
