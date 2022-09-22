#pragma once

#include "engine/base/panic.h"

#include <type_traits>
#include <string_view>

#include "dx/d3d12.h"
#include <dxgi1_6.h>

#define DX_CBUFFER alignas(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT)
#define CHECK(expr) ASSERT(SUCCEEDED(expr))

namespace engine::render {
    template<typename T>
    concept IsObjectName = std::is_convertible_v<T, std::string_view>;

    template<typename T>
    concept IsComObject = std::is_base_of_v<IUnknown, T>;

    template<typename T>
    concept IsD3D12Object = std::is_base_of_v<ID3D12Object, T>;

    constexpr auto kComRelease = [](IUnknown *ptr) {
        ptr->Release();
    };

    template<IsComObject T>
    struct Com {
        using Self = T;

        Com(T *self = nullptr) : self(self) { }
        Com(Com &&other) : self(other.self) { other.self = nullptr; }

        ~Com() { tryDrop(); }

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
            auto refs = release();
            ASSERT(refs == 0);
        }

        void tryDrop() {
            if (!valid()) { return; }
            drop();
        }

        ULONG release() { return self->Release(); }

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
