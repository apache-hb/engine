#pragma once

#include <atomic>
#include <memory>
#include <ranges>
#include <cstdint>
#include <cstddef>
#include <concepts>
#include <optional>

namespace ubn {
    constexpr std::size_t const hardware_constructive_interference_size { 2 * sizeof(std::max_align_t) };
    
    template <typename T>
    class queue {
    public:
        queue() = delete;
        
        constexpr explicit queue(const std::size_t capacity = UINT16_MAX) :
        m_capacity(capacity), m_allocator(std::allocator<container_t<T>>()), m_head(0ul), m_tail(0ul) {
            m_container = m_allocator.allocate(m_capacity + 1);
            for (const auto& i : std::views::iota(0ul, m_capacity)) { new(&m_container[i]) container_t<T>(); }
        }
        
        constexpr queue           (const queue&) = delete;
        constexpr queue &operator=(const queue&) = delete;
        
        ~queue(void) noexcept {
            for (const auto& i : std::views::iota(0ul, m_capacity)) { m_container[i].~container_t(); }
            m_allocator.deallocate(m_container, m_capacity + 1);
        }
        
        constexpr void push(const T&                      v) noexcept { emplace(v); }
        constexpr void push(std::convertible_to<T> auto&& v) noexcept { emplace(std::forward<decltype(v)>(v)); }
        
        constexpr bool is_empty() const noexcept { return m_head == m_tail; }

        constexpr T pop() noexcept {
            std::optional<T> res;
            
            const auto tail      { m_tail.fetch_add(1, std::memory_order::acquire) };
            const auto head      { (tail / m_capacity) * 2 + 1 };
            auto*      container { &m_container[tail % m_capacity] };
            
            while (true) {
                const auto now { container->m_idx_.load(std::memory_order::acquire) };
                if (now == head) { break; }
                container->m_idx_.wait(now, std::memory_order::relaxed);
            }
            
            res = std::move(container->move());
            container->destruct();
            
            container->m_idx_.store(head + 1, std::memory_order::release);
            container->m_idx_.notify_all();
            
            return std::move(*res);
        }
        
    protected:
        template <typename... Args>
        constexpr void emplace(Args&&... args) noexcept {
            const auto tail      { m_head.fetch_add(1, std::memory_order::acquire) };
            const auto head      { (tail / m_capacity) * 2 };
            auto*      container { &m_container[tail % m_capacity] };
            
            
            while (true) {
                const auto now { container->m_idx_.load(std::memory_order::acquire) };
                if (now == head) { break; }
                container->m_idx_.wait(now, std::memory_order::relaxed);
            }
            
            container->construct(std::forward<Args>(args)...);
            
            container->m_idx_.store(head + 1, std::memory_order::release);
            container->m_idx_.notify_all();
        }
        
        template <typename V>
        struct container_t {
        public:
            ~container_t() noexcept { if (m_idx_.load(std::memory_order::acquire)) { destruct(); } }
            
            template <typename... Args>
            constexpr void construct(Args&&... args) noexcept { new(&storage) V(std::forward<Args>(args)...); }
            constexpr void destruct ()               noexcept { reinterpret_cast<V*>(&storage)->~V(); }
            constexpr V&&  move     ()               noexcept { return reinterpret_cast<V&&>(storage); }
            
            alignas(hardware_constructive_interference_size) std::atomic_size_t mutable m_idx_ { 0 };
            
        private:
            alignas(V) std::byte storage[sizeof(V)];
        };
        
    private:
        const std::size_t              m_capacity;
        std::allocator<container_t<T>> m_allocator;
        container_t<T>*                m_container;
        
        alignas(hardware_constructive_interference_size) std::atomic_size_t m_head;
        alignas(hardware_constructive_interference_size) std::atomic_size_t m_tail;
    };
}
