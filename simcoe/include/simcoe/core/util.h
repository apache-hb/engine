#pragma once

#include <atomic>
#include <string_view>
#include <span>
#include <unordered_set>

namespace simcoe::util {
    std::string narrow(std::wstring_view wstr);
    std::wstring widen(std::string_view str);

    std::string join(std::string_view sep, std::span<const std::string> parts);

    struct DoOnce {
        void reset() { done = false; }
        bool check() const { return done; }
        operator bool() const { return done; }

        template<typename F>
        void operator()(F&& f) {
            if (done) { return; }
            done = true;
            f();
        }
    private:
        std::atomic_bool done = false;
    };

    template<typename T>
    struct DoOnceGroup {
        void reset(const T& it) { done.erase(it); }

        template<typename F>
        void operator()(const T& it, F&& f) {
            if (const auto& [iter, inserted] = done.insert(it); !inserted) { return; }

            f();
        }
    private:
        std::unordered_set<T> done;
    };
}
