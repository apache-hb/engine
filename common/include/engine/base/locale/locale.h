#pragma once

#include <string>
#include <string_view>
#include <unordered_map>

namespace simcoe::locale {
    enum Lang {
        eEnglish,
        ePolish
    };

    struct Locale {
        Lang lang;
        std::unordered_map<std::string, std::string> keys;
    };

    void set(Locale *current);
    std::string_view get(std::string_view key, const Locale *locale = nullptr);
    Locale *load(Lang lang, std::string_view path);
}
