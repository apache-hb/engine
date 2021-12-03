#pragma once

#include "util/win32.h"
#include "logging/log.h"

namespace engine::system {
    extern engine::logging::Channel *system;

    struct Stats {
        Stats();

        std::string_view name() const;

    private:
        std::string id;
    };
}
