#pragma once

#include <atomic>

namespace simcoe::util {
    template<typename T>
    struct Progress {
        constexpr Progress(T total = ~T(0))
            : total(total)
        { }

        constexpr Progress(const Progress& other)
            : current(other.current)
            , total(other.total)
        { }

        constexpr Progress& operator=(const Progress& other) {
            current.store(other.current);
            total = other.total;
            return *this;
        }

        constexpr void update(T it) { current = it; }
        constexpr bool done() const { return current == total; }
        constexpr float fraction() const { return float(current) / float(total); }
    private:
        std::atomic<T> current = 0;
        T total = 0;
    };

    // iterate over a range of values, updating a progress object
    constexpr auto progress(Progress<size_t>& progress, size_t size) {
        struct Range {
            Progress<size_t>& progress;
            size_t size;

            struct Iterator {
                Progress<size_t>& progress;
                size_t size;
                size_t i = 0;
                size_t operator*() const { return i; }
                Iterator& operator++() { ++i; progress.update(i); return *this; }
                bool operator!=(const Iterator& other) const { return i != other.i; }
            };

            Iterator begin() { return { progress, size, 0 }; }
            Iterator end() { return { progress, size, size }; }
        };

        progress = Progress<size_t>(size);

        return Range { progress, size };
    }
}