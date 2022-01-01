#pragma once

#include "objects/factory.h"
#include "system/system.h"

namespace engine::render {
    namespace Resource {
        enum Index : int {
            SceneTarget,
            ImGui,
            Total
        };
    }

    namespace Allocator {
        enum Index : int {
            Target,
            Copy,
            Total
        };

        constexpr D3D12_COMMAND_LIST_TYPE getType(Index index) {
            switch (index) {
            case Target: return D3D12_COMMAND_LIST_TYPE_DIRECT;
            case Copy: return D3D12_COMMAND_LIST_TYPE_COPY;
            default: throw engine::Error("invalid allocator index");
            }
        }

        constexpr std::wstring_view getName(Index index) {
            switch (index) {
            case Target: return L"target";
            case Copy: return L"copy";
            default: throw engine::Error("invalid allocator index");
            }
        }
    }

    struct FrameData {
        UINT64 fenceValue;
        Object<ID3D12Resource> target;
        Object<ID3D12CommandAllocator> allocators[Allocator::Total];
    };

    struct Context {
        struct Create {
            Factory* factory;
            size_t adapter;
            system::Window* window;
            UINT buffers;
            Resolution resolution;
        };

        Context(Create&& create);
        ~Context();

        bool present();

    private:
        Create info;

        void createDevice();
        void destroyDevice();

        void createViews();

        void createDirectCommandQueue();
        void destroyDirectCommandQueue();

        void createUploadCommandQueue();
        void destroyUploadCommandQueue();

        void createSwapChain();
        void destroySwapChain();

        void createRtvHeap();
        void destroyRtvHeap();

        void createCbvHeap();
        void destroyCbvHeap();

        void createSceneTarget();
        void destroySceneTarget();

        void createFrameData();
        void destroyFrameData();

        void createFence();
        void destroyFence();



        Factory& getFactory();
        Adapter& getAdapter();
        system::Window& getWindow();
        Resolution getDisplayResolution() const;
        Resolution getInternalResolution();
        DXGI_FORMAT getFormat() const;
        UINT getBufferCount() const;

        UINT getCurrentFrame() const;

        UINT requiredRtvHeapSize() const;
        UINT requiredCbvHeapSize() const;

        D3D12_CPU_DESCRIPTOR_HANDLE sceneTargetRtvCpuHandle();
        D3D12_CPU_DESCRIPTOR_HANDLE sceneTargetCbvCpuHandle();

        D3D12_CPU_DESCRIPTOR_HANDLE rtvHeapCpuHandle(UINT index);

        Object<ID3D12CommandAllocator> getAllocator(Allocator::Index index);
        Object<ID3D12Resource> getTarget();


        Device<ID3D12Device4> device;

        Object<ID3D12CommandQueue> directCommandQueue;
        Object<ID3D12CommandQueue> copyCommandQueue;

        Com<IDXGISwapChain3> swapchain;

        DescriptorHeap rtvHeap;
        DescriptorHeap cbvHeap;

        Object<ID3D12Resource> sceneTarget;

        FrameData *frameData;

        UINT frameIndex;


        Object<ID3D12Fence> fence;
        HANDLE fenceEvent;


        View sceneView;
        View postView;

        Resolution displayResolution;
        Resolution sceneResolution;
    };
}
