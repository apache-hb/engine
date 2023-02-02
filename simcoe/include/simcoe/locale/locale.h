#pragma once

#include "simcoe/core/logging.h"
#include "simcoe/input/input.h"
#include <unordered_map>

namespace simcoe {
    struct Locale {
        using KeyMap = std::unordered_map<std::string, std::string>;
        
        Locale();
        // Locale(const char *pzFile);
        Locale(const KeyMap& map);

        // logging
        const char *get(logging::Level it) const;

        // input
        const char *get(input::Device it) const;
        const char *get(input::Key it) const;
        const char *get(input::Axis it) const;

        const char *get(const char *key) const;

    private:
        KeyMap strings;
    };
}
