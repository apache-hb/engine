#pragma once

#include <span>

#include "util.h"
#include "system/system.h"
#include "assets/loader.h"
#include "input/camera.h"

namespace engine::render {
    namespace Slots {
        enum Index : int {
            Buffer = 0,
            Texture = 1,
            ImGui = 2,
            Total
        };
    }

    namespace Allocator {
        enum Index : int {
            Context = 0,
            Copy = 1,
            Total
        };

        constexpr D3D12_COMMAND_LIST_TYPE getType(Index index) {
            switch (index) {
            case Context: return D3D12_COMMAND_LIST_TYPE_DIRECT;
            case Copy: return D3D12_COMMAND_LIST_TYPE_COPY;
            default: throw engine::Error("invalid allocator index");
            }
        }

        constexpr std::wstring_view getName(Index index) {
            switch (index) {
            case Context: return L"context";
            case Copy: return L"copy";
            default: throw engine::Error("invalid allocator index");
            }
        }
    }

    namespace debug {
        void enable();
        void disable();
        void report();
    }

    struct Context {
        struct Create {
            Adapter adapter;
            system::Window *window;
            UINT frames;
        };
        
        Context(Factory factory)
            : factory(factory)
        { }

        ~Context() {
            destroyAssets();
            destroyDevice();
        }

        void createDevice(Create& info);
        void destroyDevice();

        void createAssets();
        void destroyAssets();

        void present();
        void tick(const input::Camera& camera);

        ConstBuffer constBufferData;

    private:
        void attachInfoQueue();
        D3D_ROOT_SIGNATURE_VERSION rootVersion();

        void populate();
        void flushQueue();
        void flushCopyQueue();

        void waitForGPU(Com<ID3D12CommandQueue> queue);
        void nextFrame(Com<ID3D12CommandQueue> queue);

        Factory factory;
        Create create;

        struct View {
            d3d12::Scissor scissor{0, 0};
            d3d12::Viewport viewport{0.f, 0.f};
        };

        struct Frame {
            Com<ID3D12Resource> target;
            Com<ID3D12CommandAllocator> allocators[Allocator::Total];
            UINT64 fenceValue;
        };

        Com<ID3D12Resource> uploadBuffer(const void *data, size_t size, std::wstring_view name = L"resource");
        Com<ID3D12Resource> uploadTexture(const loader::Texture& texture, const D3D12_CPU_DESCRIPTOR_HANDLE& handle, std::wstring_view name = L"texture");

        template<typename T>
        Com<ID3D12Resource> uploadSpan(const std::span<T>& data, std::wstring_view name = L"resource") {
            return uploadBuffer(data.data(), data.size() * sizeof(T), name);
        }

        View scene;

        /// pipeline objects
        Com<ID3D12Device> device;

        Com<ID3D12CommandQueue> directQueue;

        Com<IDXGISwapChain3> swapchain;
        float aspect;

        Com<ID3D12RootSignature> rootSignature;
        Com<ID3D12PipelineState> pipelineState;

        Com<ID3D12DescriptorHeap> dsvHeap;
        
        Com<ID3D12DescriptorHeap> rtvHeap;
        UINT rtvDescriptorSize;

        Com<ID3D12DescriptorHeap> cbvSrvHeap;
        UINT cbvSrvDescriptorSize;

        auto getCbvSrvGpuHandle(UINT index) {
            return CD3DX12_GPU_DESCRIPTOR_HANDLE(cbvSrvHeap->GetGPUDescriptorHandleForHeapStart(), index, cbvSrvDescriptorSize);
        }

        auto getCbvSrvCpuHandle(UINT index) {
            return CD3DX12_CPU_DESCRIPTOR_HANDLE(cbvSrvHeap->GetCPUDescriptorHandleForHeapStart(), index, cbvSrvDescriptorSize);
        }

        Com<ID3D12GraphicsCommandList> commandList;

        Com<ID3D12CommandQueue> copyQueue;
        Com<ID3D12GraphicsCommandList> copyCommandList;
        
        std::vector<Com<ID3D12Resource>> copyResources;

        /// resources
        Com<ID3D12Resource> vertexBuffer;
        D3D12_VERTEX_BUFFER_VIEW vertexBufferView;

        Com<ID3D12Resource> indexBuffer;
        D3D12_INDEX_BUFFER_VIEW indexBufferView;

        Com<ID3D12Resource> constBuffer;
        void *constBufferPtr;

        Com<ID3D12Resource> depthStencil;

        Com<ID3D12Resource> texture;

        loader::Model model;

        /// frame data
        Frame *frames;

        auto &getAllocator(Allocator::Index type, size_t index = SIZE_MAX) noexcept {
            if (index == SIZE_MAX) { index = frameIndex; }
            return frames[index].allocators[type];
        } 

        auto &getTarget(size_t index = SIZE_MAX) noexcept {
            if (index == SIZE_MAX) { index = frameIndex; }
            return frames[index].target;
        }

        /// sync objects
        Com<ID3D12Fence> fence;
        HANDLE fenceEvent;
        UINT frameIndex;
    };
}
