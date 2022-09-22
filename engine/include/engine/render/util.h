#pragma once

#include <type_traits>
#include <string_view>

#include "dx/d3d12.h"
#include <dxgi1_6.h>

namespace engine::render {
    template<typename T>
    concept IsObjectName = std::is_convertible_v<T, std::string_view>;

    template<typename T>
    concept IsComObject = std::is_convertible_v<T*, IUnknown*>;

    template<typename T>
    concept IsD3D12Object = std::is_convertible_v<T*, ID3D12Object*>;

    template<IsComObject T>
    struct Com {
        using Self = T;

        Com(T *self = nullptr) : self(self) { }
        Com(Com &&other) : self(other.self) { other.self = nullptr; }

        Com(const Com&) = delete;
        Com &operator=(const Com&) = delete;

        Com &operator=(Com&& other) {
            self = other.self;
            other.self = nullptr;
            return *this;
        }

        operator bool() const { return self != nullptr; }
        bool valid() const { return self != nullptr; }

        T* operator->() { return self; }
        T* operator*() { return self; }
        T** operator&() { return &self; }

        const T* operator->() const { return self; }

        T *get() { return self; }
        T **addr() { return &self; }

        const T *get() const { return self; }
        const T **addr() const { return &self; }
        
        void drop() {
            release();
        }

        void tryDrop() {
            if (!valid()) { return; }
            drop();
        }

        auto release() { return self->Release(); }

        template<IsComObject O>
        Com<O> as() {
            O *other = nullptr;
            
            if (HRESULT hr = self->QueryInterface(IID_PPV_ARGS(&other)); FAILED(hr)) {
                return Com<O>();
            }

            // QueryInterface adds a reference, so we need to release it
            release();
            return Com<O>(other);
        }

    private:
        T *self;
    };

    template<IsD3D12Object T>
    struct Object : Com<T> {
        using Super = Com<T>;
        using Super::Super;
    };
}
