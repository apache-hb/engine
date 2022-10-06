#pragma once

#include <string>
#include <string_view>
#include <sstream>
#include <span>

namespace engine {
    namespace strings {
        std::string narrow(std::wstring_view str);
        std::wstring widen(std::string_view str);

        template<typename T>
        std::string join(std::span<T> items, std::string_view sep) {
            std::stringstream ss;
            for (size_t i = 0; i < items.size(); i++) {
                if (i != 0) {
                    ss << sep;
                }
                ss << items[i];
            }
            return ss.str();
        }
    }

    struct Timer {
        Timer();
        double tick();

    private:
        size_t last;
        size_t frequency;
    };

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

    template<typename T, typename Delete> requires (std::is_trivially_copy_constructible<T>::value)
    struct UniqueResource {
        UniqueResource(T object) { 
            init(object);
        }

        UniqueResource(UniqueResource &&other) { 
            init(other.claim());
        }

        UniqueResource &operator=(UniqueResource &&other) {
            destroy();
            init(other.claim());
            return *this;
        }

        UniqueResource(const UniqueResource&) = delete;
        UniqueResource &operator=(const UniqueResource&) = delete;

        ~UniqueResource() {
            destroy();
        }

        T get() { return value(); }

    private:
        void destroy() {
            if (valid) {
                Delete{}(data);
            }
            valid = false;
        }

        T claim() {
            valid = false;
            return value();
        }

        T value() {
            return data;
        }

        void init(T it) {
            data = it;
            valid = true;
        }

        T data;
        bool valid = false;
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
            return *this;
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
