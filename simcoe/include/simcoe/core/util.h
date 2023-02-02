#pragma once

#include <string_view>

namespace simcoe::util {
    std::string narrow(std::wstring_view wstr);
    std::wstring widen(std::string_view str);

    struct DoOnce {
        void reset() { done = false; }

        template<typename F>
        void operator()(F&& f) {
            if (done) { return; }
            done = true;
            f();
        }
    private:
        bool done = false;
    };
}
