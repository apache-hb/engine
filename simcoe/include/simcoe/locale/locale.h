#pragma once

#include "simcoe/core/logging.h"
#include "simcoe/input/input.h"

namespace simcoe {
    struct Locale {
        // logging
        const char *get(logging::Level it) const;

        // input
        const char *get(input::Device it) const;
        const char *get(input::Key it) const;
        const char *get(input::Axis it) const;

        const char *get(const char *key) const;
    };
}
