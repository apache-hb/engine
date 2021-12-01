#include "system.h"

namespace engine::system {
    engine::logging::Channel *system = new engine::logging::ConsoleChannel("system", stdout);

    Stats::Stats() 
        : memory({ .dwLength = sizeof(MEMORYSTATUSEX) })
        , id(MAX_COMPUTERNAME_LENGTH + 1, ' ')
    {
        system->check(GlobalMemoryStatusEx(&memory) != 0, win32::Win32Error("global-memory-status"));
    
        DWORD size = MAX_COMPUTERNAME_LENGTH + 1;
        system->check(
            GetComputerNameA(id.data(), &size) != 0, 
            win32::Win32Error("get-computer-name")
        );

        id.resize(size);
    }

    Stats::Memory Stats::totalPhysical() const {
        return memory.ullTotalPhys;
    }

    Stats::Memory Stats::availPhysical() const {
        return memory.ullAvailPhys;
    }

    Stats::Memory Stats::totalVirtual() const {
        return memory.ullTotalVirtual;
    }

    Stats::Memory Stats::availVirtual() const {
        return memory.ullAvailVirtual;
    }

    std::string_view Stats::name() const {
        return id;
    }
}
