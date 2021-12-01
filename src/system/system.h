#pragma once

#include "util/win32.h"
#include "logging/log.h"
#include "util/units.h"

namespace engine::system {
    extern engine::logging::Channel *system;

    struct Stats {
        using Memory = engine::units::Memory;

        Stats();

        Memory totalPhysical() const;
        Memory availPhysical() const;
        Memory totalVirtual() const;
        Memory availVirtual() const; 
        std::string_view name() const;

    private:
        std::string id;
        MEMORYSTATUSEX memory;
    };
}
