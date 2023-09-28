#pragma once

#include <atomic>
#include <memory>
#include <string_view>
#include <span>
#include <unordered_map>
#include <unordered_set>
#include <mutex>

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

    struct RegistryBase;

    struct Entry {
        Entry(RegistryBase& base);
        virtual ~Entry();

        virtual void apply() = 0;

    private:
        RegistryBase& base;
    };

    struct RegistryBase {
        std::mutex mutex;
        std::unordered_set<Entry*> entries = {};
    };

    template<typename T>
    struct Registry : RegistryBase {
        template<typename F>
        std::unique_ptr<Entry> newEntry(const T& data, F&& func) {
            struct EntryImpl : Entry {
                EntryImpl(RegistryBase& base, F&& func)
                    : Entry(base)
                    , func(std::move(func)) 
                { }

                void apply() override { func(); }
            private:
                F func;
            };

            Entry *pImpl = new EntryImpl(*this, std::move(func));
            extra.emplace(pImpl, data);
            return std::unique_ptr<Entry>(pImpl);
        }

        const T& get(const Entry* entry) const {
            return extra.at(entry);
        }
        
        std::unordered_map<const Entry*, T> extra;
    };
}
