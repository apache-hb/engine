#pragma once

#include "simcoe/core/panic.h"

#include <coroutine>
#include <xutility>

namespace simcoe::async {
    template<typename T>
    struct Generator {
        struct Promise;
        struct Iterator;

        using promise_type = Promise;
        using handle_type = std::coroutine_handle<promise_type>;

        struct Promise {
            auto get_return_object() noexcept { return Generator(handle_type::from_promise(*this)); }
            auto initial_suspend() noexcept { return std::suspend_always(); }
            auto final_suspend() noexcept { return std::suspend_always(); }
            void unhandled_exception() noexcept { ASSERT(false); }

            auto yield_value(T value) noexcept {
                current = value;
                return std::suspend_always{};
            }

            void return_void() noexcept { }

        private:
            friend struct Iterator;
            T current;
        };

        struct Iterator {
            Iterator& operator++() {
                coro.resume();
                return *this;
            }

            T& operator*() { return coro.promise().current; }
            
            bool operator==(std::default_sentinel_t) const { return coro.done(); }

            Iterator(handle_type& coro)
                : coro(coro)
            { }
        private:
            handle_type coro;
        };

        Iterator begin() { 
            coro.resume();
            return {coro}; 
        }

        auto end() { return std::default_sentinel_t{}; }

        T current_value() { return coro.promise().current; }

        Generator(handle_type coro) 
            : coro(coro) 
        { }

        ~Generator() {
            if (coro) {
                coro.destroy();
            }
        }

    private:
        handle_type coro;
    };
}
