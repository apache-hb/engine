#pragma once

#include "d3dx12.h"
#include <dxgi1_6.h>
#include <vector>

namespace engine::render {
    template<typename T>
    struct Com {
        using Self = T;
        Com(T *self = nullptr) : self(self) { }

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
        Com<IDXGIOutput> self;
        DXGI_OUTPUT_DESC desc;
    };

    struct Adapter {
        Com<IDXGIAdapter> self;
        DXGI_ADAPTER_DESC desc;
        std::vector<Output> outputs;
    };

    struct Factory {
        Com<IDXGIFactory> self;
        std::vector<Adapter> adapters;
        bool tearing;
    };
}
