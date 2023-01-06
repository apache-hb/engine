#include "engine/base/locale/locale.h"

#include "engine/base/logging.h"
#include "engine/base/panic.h"

#include <format>
#include <fstream>
#include <unordered_map>

using namespace simcoe;
using namespace simcoe::locale;

namespace {
    void trim(std::string& str) {
        while (str.starts_with(' ')) {
            str.erase(0, 1);
        }
        while (str.ends_with(' ')) {
            str.pop_back();
        }
    }

    Locale *current = nullptr;

    std::string_view id(Lang lang) {
        switch (lang) {
        case Lang::eEnglish: return "English";
        case Lang::ePolish: return "Polish";
        default: return "unknown";
        }
    }
}

std::string_view locale::get(std::string_view key, const Locale *locale) {
    const Locale *it = locale ? locale : current;
    // TODO: an allocation for every query? really?
    // whoever designed the hashmap interface is a moron
    auto name = std::string(key);
    auto& channel = logging::get(logging::eLocale);
    if (!it->keys.contains(name)) {
        channel.warn("Missing key `{}` in locale `{}`", key, id(it->lang));
        return key;
    }
    return it->keys.at(name);
}

void locale::set(Locale *it) {
    logging::get(logging::eLocale).info("setting current locale to {}", id(it->lang));
    current = it;
}

Locale *locale::load(Lang lang, std::string_view path) {
    std::ifstream input(std::string(path).c_str());
    auto& channel = logging::get(logging::eLocale);

    Locale *it = new Locale { lang, { } };

    if (!input.is_open()) {
        channel.warn("failed to load locale file: {}", path);
        return it;
    }

    channel.info("loading locale {}", path);

    while (!input.eof()) {
        std::string line;
        std::getline(input, line);
        auto i = line.find(':');
        if (i == std::string::npos) {
            continue;
        }
        auto key = line.substr(0, i);
        auto val = line.substr(i + 1, line.length() - i - 1);

        trim(key);
        trim(val);

        it->keys[key] = val;
    }

    if (current == nullptr) {
        set(it);
    }

    return it;
}
