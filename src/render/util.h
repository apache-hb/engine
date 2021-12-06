#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <dxgidebug.h>
#include <vector>
#include "d3dx12.h"

#include "logging/log.h"
#include "util/error.h"

namespace engine::render {
    extern logging::Channel *render;

    template<typename T>
    using Result = engine::Result<T, HRESULT>;

    std::string to_string(HRESULT hr);
    
    namespace debug {
        HRESULT enable();
        HRESULT report();
        void disable();
    }

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

    namespace dxgi {
        using Debug = Com<IDXGIDebug>;

        using Factory4 = Com<IDXGIFactory4>;
        using FactoryFlags = UINT;

        using Adapter1 = Com<IDXGIAdapter1>;
        using AdapterDesc1 = DXGI_ADAPTER_DESC1;

        using SwapChain1 = Com<IDXGISwapChain1>;
        using SwapChain3 = Com<IDXGISwapChain3>;
    }

    namespace d3d12 {
        using Debug = Com<ID3D12Debug>;
        using Device1 = Com<ID3D12Device1>;
        using Device5 = Com<ID3D12Device5>;
        using InfoQueue1 = Com<ID3D12InfoQueue1>;
        using CommandQueue = Com<ID3D12CommandQueue>;
        using DescriptorHeap = Com<ID3D12DescriptorHeap>;
        using Resource = Com<ID3D12Resource>;
        using CommandAllocator = Com<ID3D12CommandAllocator>;
        using CommandList = Com<ID3D12GraphicsCommandList>;
        using Fence = Com<ID3D12Fence>;

        struct Viewport : D3D12_VIEWPORT {
            using Super = D3D12_VIEWPORT;
            Viewport(FLOAT width, FLOAT height)
                : Super({ 0.f, 0.f, width, height, 0.f, 1.f })
            { }
        };

        struct Scissor : D3D12_RECT {
            using Super = D3D12_RECT;
            Scissor(LONG width, LONG height)
                : Super({ 0, 0, width, height })
            { }
        };
    }

    struct Adapter {
        auto get() { return adapter.get(); }

        dxgi::Adapter1 adapter;
        dxgi::AdapterDesc1 desc;
    };

    Result<Adapter> createAdapter(dxgi::Adapter1 adapter);

    struct Factory {
        auto operator->() { return factory.get(); }
        auto get() { return factory.get(); }

        dxgi::Factory4 factory;
        std::vector<Adapter> adapters;
    };

    Result<Factory> createFactory();

    struct ImGuiChannel : logging::Channel {
        ImGuiChannel(std::string_view name)
            : Channel(name, false)
        { }

        virtual void send(Level report, std::string_view message) override;
    };
}
