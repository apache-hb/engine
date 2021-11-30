#pragma once

#include "../utils.h"

#include <d3d12.h>
#include "d3dx12.h"
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

    namespace dxgi {
        using Factory4 = Ptr<IDXGIFactory4>;
        using Adapter1 = Ptr<IDXGIAdapter1>;
        using SwapChain1 = Ptr<IDXGISwapChain1>;
        using SwapChain3 = Ptr<IDXGISwapChain3>;
    }

    namespace d3d {
        using Blob = Ptr<ID3DBlob>;
    }

    namespace d3d12 {
        using Device1 = Ptr<ID3D12Device1>;
        using CommandQueue = Ptr<ID3D12CommandQueue>;
        using InfoQueue1 = Ptr<ID3D12InfoQueue1>;
        using Debug = Ptr<ID3D12Debug>;
        using Fence = Ptr<ID3D12Fence>;
        using DescriptorHeap = Ptr<ID3D12DescriptorHeap>;
        using CommandAllocator = Ptr<ID3D12CommandAllocator>;
        using Resource = Ptr<ID3D12Resource>;
        using RootSignature = Ptr<ID3D12RootSignature>;
        using PipelineState = Ptr<ID3D12PipelineState>;
        using CommandList = Ptr<ID3D12GraphicsCommandList>;
    
        struct DescriptorHeapRTV : DescriptorHeap {
            UINT descriptorSize;
        };

        struct Commands : CommandList {
            template<typename F>
            HRESULT record(F &&func) {
                if (HRESULT hr = func(get()); FAILED(hr)) {
                    fprintf(stderr, "d3d12: CommandList::record = 0x%x\n", hr);
                    return hr;
                }
                return get()->Close();
            }

            void transition(
                d3d12::Resource resource, 
                D3D12_RESOURCE_STATES from,
                D3D12_RESOURCE_STATES to
            )
            {
                const auto transition = CD3DX12_RESOURCE_BARRIER::Transition(resource.get(), from, to);
                get()->ResourceBarrier(1, &transition);
            }
        };

        struct Viewport : D3D12_VIEWPORT {
            using Super = D3D12_VIEWPORT;

            Viewport(FLOAT width, FLOAT height)
                : Super({ 0.f, 0.f, width, height, 0.f, 0.f })
            { 
                ASSERT(width >= 0.f);
                ASSERT(height >= 0.f);
            }
        };

        struct Scissor : D3D12_RECT {
            using Super = D3D12_RECT;

            Scissor(LONG width, LONG height)
                : Super({ 0, 0, width, height })
            { }
        };
    }
}
