#include "system.h"

namespace engine::system {
    engine::logging::Channel *system = new engine::logging::ConsoleChannel("system", stdout);

    Stats::Stats() : id(MAX_COMPUTERNAME_LENGTH + 1, ' ') {
        DWORD size = MAX_COMPUTERNAME_LENGTH + 1;
        system->check(
            GetComputerNameA(id.data(), &size) != 0, 
            win32::Win32Error("get-computer-name")
        );

        id.resize(size);
    }

    std::string_view Stats::name() const { return id; }
}
