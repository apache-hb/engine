#include "strings.h"

#include <stdarg.h>

#include <sstream>

namespace strings {
    std::string join(std::span<std::string> parts, std::string_view sep) {
        std::stringstream ss;
        for (size_t i = 0; i < parts.size(); i++) {
            if (i != 0) { ss << sep; }
            ss << parts[i];
        }
        return ss.str();
    }

    std::string cformat(const char *fmt, ...) {
        va_list args;
        va_start(args, fmt);

        auto result = cformatv(fmt, args);

        va_end(args);

        return result;
    }

    std::string cformatv(const char *fmt, va_list args) {
        va_list again;
        va_copy(again, args);

        auto len = vsnprintf(nullptr, 0, fmt, args) + 1;

        std::string result(len, '\0');

        auto used = vsnprintf(result.data(), len, fmt, again);

        result.resize(used);

        va_end(again);

        return result;
    }

    std::vector<std::string_view> split(std::string_view str, std::string_view sep) {
        std::vector<std::string_view> result;

        size_t start = 0;
        size_t end = 0;

        while (end != std::string_view::npos) {
            end = str.find(sep, start);

            result.push_back(str.substr(start, end - start));

            start = end + sep.size();
        }

        return result;
    }
}
