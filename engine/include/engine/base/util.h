#pragma once

#include <string>
#include <string_view>

namespace engine {
    std::string narrow(std::wstring_view str);
    std::wstring widen(std::string_view str);

    template<typename T>
    struct DefaultDelete {
        void operator()(T *ptr) {
            delete ptr;
        }
    };

    template<typename T>
    struct DefaultDelete<T[]> {
        void operator()(T *ptr) {
            delete[] ptr;
        }
    };

    template<typename T>
    struct RemoveArray {
        using Type = T;
    };

    template<typename T>
    struct RemoveArray<T[]> {
        using Type = T;
    };

    template<typename T, typename Delete = DefaultDelete<T>>
    struct UniquePtr {
        using Type = typename RemoveArray<T>::Type;

        UniquePtr(size_t size) requires (std::is_array_v<T>) 
            : data(new Type[size]) 
        {}

        UniquePtr(Type *data = nullptr) 
            : data(data) 
        { }
        
        ~UniquePtr() {
            destroy();
        }

        UniquePtr(UniquePtr &&other) 
            : data(other.claim()) 
        { }

        UniquePtr &operator=(UniquePtr &&other) {
            destroy();
            data = other.claim();
        }

        UniquePtr(const UniquePtr&) = delete;
        UniquePtr &operator=(const UniquePtr&) = delete;

        Type &operator[](size_t index) requires(std::is_array_v<T>) {
            return data[index];
        }

        const Type &operator[](size_t index) const requires(std::is_array_v<T>) {
            return data[index];
        }

        Type *get() { return data; }
        const Type *get() const { return data; }

        Type **ref() { return &data; }
        const Type **ref() const { return &data; }

        Type *operator->() { return get(); }
        const Type *operator->() const { return get(); }

        Type **operator&() { return ref(); }
        const Type**operator&() const { return ref(); }

    private:
        void destroy() {
            if (data != nullptr) {
                Delete{}(data);
            }
        }

        Type *claim() {
            Type *it = data;
            data = nullptr;
            return it;
        }

        Type *data;
    };
}
