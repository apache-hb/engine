#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>

namespace render {
    template<typename T>
    struct Ptr {
        using Type = T;
        using Pointer = T*;

        Ptr(T *ptr = nullptr, LPCWSTR name = nullptr) : self(ptr) { 
            if (valid()) {
                rename(name);
            }
        }

        template<typename A>
        A as(LPCWSTR name = nullptr) {
            using Ptr = typename A::Pointer;
            
            Ptr ptr = nullptr;
            if (HRESULT hr = self->QueryInterface(IID_PPV_ARGS(&ptr)); FAILED(hr)) {
                fprintf(stderr, "d3d12: QueryInterface = 0x%x\n", hr);
                return nullptr;
            }

            return A(ptr, name);
        }

        T *operator->() { return get(); }
        T *operator*() { return get(); }
        T **operator&() { return addr(); }

        T **addr() { return &self; }
        T *get() { return self; }

        ULONG release() { return self->Release(); }
        
        void tryRelease() {
            if (valid()) {
                release();
            }
        }

        void drop(const char *name = "") { 
            if (ULONG refs = release(); refs != 0) {
                fprintf(stderr, "render: release(%s) = %u\n", name, refs);
            }
        }

        void tryDrop(const char *name = "") {
            if (valid()) {
                drop(name);
            }
        }

        bool valid() const { return self != nullptr; }

        void rename(LPCWSTR name) {
            if constexpr (std::is_base_of_v<ID3D12Object, T>) {
                self->SetName(name);
            }
        }

    private:
        T *self;
    };
}
