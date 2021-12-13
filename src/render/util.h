#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <dxgidebug.h>
#include <vector>
#include <array>
#include "d3dx12.h"

#include "imgui.h"

#include <DirectXMath.h>

#include "logging/log.h"
#include "util/error.h"

#define cbuffer struct __declspec(align(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT))

namespace engine::render {
    using namespace DirectX;

    extern logging::Channel *render;

    template<typename T>
    using Result = engine::Result<T, HRESULT>;

    std::string to_string(HRESULT hr);
    
    namespace debug {
        HRESULT enable();
        HRESULT report();
        void disable();
    }

    struct Vertex {
        XMFLOAT3 position;
        XMFLOAT4 colour;
    };

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
        using Factory5 = Com<IDXGIFactory5>;
        using FactoryFlags = UINT;

        using Adapter1 = Com<IDXGIAdapter1>;
        using AdapterDesc1 = DXGI_ADAPTER_DESC1;

        using Output = Com<IDXGIOutput>;
        using OutputDesc = DXGI_OUTPUT_DESC;

        using SwapChain1 = Com<IDXGISwapChain1>;
        using SwapChain3 = Com<IDXGISwapChain3>;
    }

    namespace d3d {
        using Blob = Com<ID3DBlob>;

        HRESULT compileFromFile(LPCWSTR path, LPCSTR entry, LPCSTR shaderModel, ID3DBlob **blob);
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
        using RootSignature = Com<ID3D12RootSignature>;
        using PipelineState = Com<ID3D12PipelineState>;
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

    struct Display {
        auto get() { return output.get(); }

        dxgi::Output output;
        dxgi::OutputDesc desc;
    };

    struct Adapter {
        auto get() { return adapter.get(); }

        dxgi::Adapter1 adapter;
        dxgi::AdapterDesc1 desc;
        std::vector<Display> outputs;
    };

    Result<Adapter> createAdapter(dxgi::Adapter1 adapter);

    struct Factory {
        auto operator->() { return factory.get(); }
        auto get() { return factory.get(); }

        dxgi::Factory4 factory;
        std::vector<Adapter> adapters;
        bool tearing;
    };

    Result<Factory> createFactory();
}
