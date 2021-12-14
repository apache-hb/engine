#pragma once

#include "d3dx12.h"
#include <dxgi1_6.h>
#include <span>

namespace engine::render {
    template<typename T>
    using Result = engine::Result<T, HRESULT>;

    template<typename T>
    struct Com {
        using Self = T;

        Com(T *self = nullptr) : self(self) { 
            if (self != nullptr) { 
                self->AddRef(); 
            }
        }

        Com(const Com &other) 
            : Com(other.self)
        { }

        Com(Com &&other) : Com(other.self) { 
            other.self = nullptr; 
        }

        ~Com() { 
            if (self != nullptr) { 
                self->Release();
            } 
        }

        operator bool() const { return self != nullptr; }
        bool valid() const { return self != nullptr; }

        T *operator->() { return self; }
        T *operator*() { return self; }
        T **operator&() { return &self; }

        T *get() { return self; }
        T **addr() { return &self; }

        void drop(std::string_view name = "") {
            auto refs = release();
            if (refs > 0) {
                render->fatal("failed to drop {}, {} references are still held", name, refs);
            }
        }

        void tryDrop(std::string_view name = "") {
            if (!valid()) { return; }
            drop(name);
        }

        auto release() { return self->Release(); }

        template<typename O>
        Result<O> as() {
            using Other = typename O::Self;

            Other *other = nullptr;
            if (HRESULT hr = self->QueryInterface(IID_PPV_ARGS(&other)); FAILED(hr)) {
                return fail(hr);
            }

            return pass(O(other));
        }
    private:
        T *self;
    };

    struct Output {
        std::string_view name() const;

    private:
        Com<IDXGIOutput> output;
        DXGI_OUTPUT_DESC desc;
    };

    struct Adapter {
        std::string_view name() const;
        std::span<Output> outputs();

    private:
        Com<IDXGIAdapter2> self;
        DXGI_ADAPTER_DESC desc;
        std::vector<Output> currentOutputs;
    };

    struct Factory {
        std::span<Adapter> adapters();
        bool tearing() const;

    private:
        Com<IDXGIFactory4> self;
        std::vector<Adapter> currentAdapters;
        bool tearingSupport;
    };
}
