#pragma once

#include "system/system.h"
#include "render/objects/device.h"

namespace engine::render {
    struct DisplayViewport;
    struct Scene;
    struct Factory;
    struct Adapter;

    constexpr float kClearColour[] = { 0.0f, 0.2f, 0.4f, 1.0f };

    namespace Resources {
        enum Index : int {
            SceneTarget,
            ImGui,
            Total
        };
    }

    namespace Allocator {
        enum Index : int {
            Viewport,
            Scene,
            Copy,
            Total
        };

        constexpr D3D12_COMMAND_LIST_TYPE getType(Index index) {
            switch (index) {
            case Viewport: case Scene: return D3D12_COMMAND_LIST_TYPE_DIRECT;
            case Copy: return D3D12_COMMAND_LIST_TYPE_COPY;
            default: throw engine::Error("invalid allocator index");
            }
        }

        constexpr std::wstring_view getName(Index index) {
            switch (index) {
            case Viewport: return L"viewport";
            case Scene: return L"scene";
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

    struct TextureData {
        UINT width;
        UINT height;
        UINT component;
        std::span<const uint8_t> data;
    };

    struct Context {
        struct Create {
            Factory* factory;
            size_t adapter;
            system::Window* window;
            UINT buffers;
            Resolution resolution;

            DisplayViewport*(*getViewport)(Context*);
            Scene*(*getScene)(Context*);
        };

        Context(Create&& create);

        void create();
        void destroy();

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

        void createCopyCommandList();
        void destroyCopyCommandList();

        void createCopyFence();
        void destroyCopyFence();

        void waitForGpu();
        void nextFrame();

        void finishCopy();

    public:
        Resource uploadData(std::wstring_view name, const void* data, size_t size);
        Resource uploadTexture(std::wstring_view name, const TextureData& data);

        template<typename T>
        VertexBuffer uploadVertexBuffer(std::wstring_view name, std::span<const T> data) {
            auto resource = uploadData(name, data.data(), data.size_bytes());
            D3D12_VERTEX_BUFFER_VIEW view = { 
                .BufferLocation = resource.gpuAddress(), 
                .SizeInBytes = UINT(data.size_bytes()), 
                .StrideInBytes = sizeof(T) 
            };
            return { resource, view };
        }

        Factory& getFactory();
        Adapter& getAdapter();
        Device<ID3D12Device4>& getDevice();
        system::Window& getWindow();
        Resolution getDisplayResolution();
        Resolution getInternalResolution();
        DXGI_FORMAT getFormat() const;
        UINT getBufferCount() const;

        View getPostView() const;
        View getSceneView() const;

        UINT getCurrentFrame() const;

        UINT requiredRtvHeapSize() const;
        UINT requiredCbvHeapSize() const;

        D3D12_CPU_DESCRIPTOR_HANDLE sceneTargetRtvCpuHandle();
        D3D12_CPU_DESCRIPTOR_HANDLE sceneTargetCbvCpuHandle();

        D3D12_CPU_DESCRIPTOR_HANDLE rtvHeapCpuHandle(UINT index);

        Object<ID3D12CommandAllocator>& getAllocator(Allocator::Index index);
        Object<ID3D12Resource> getTarget();
        Object<ID3D12Resource> getSceneTarget();
        DescriptorHeap getCbvHeap();

    private:
        Device<ID3D12Device4> device;

        Object<ID3D12CommandQueue> directCommandQueue;
        Object<ID3D12CommandQueue> copyCommandQueue;

        Object<ID3D12GraphicsCommandList> copyCommandList;

        std::vector<Resource> copyResources;

        Com<IDXGISwapChain3> swapchain;

        DescriptorHeap rtvHeap;
        DescriptorHeap cbvHeap;

        Object<ID3D12Resource> sceneTarget;

        Scene* scene;
        DisplayViewport* display;

        FrameData* frameData;

        UINT frameIndex;

        Object<ID3D12Fence> fence;
        HANDLE fenceEvent;

        Object<ID3D12Fence> copyFence;
        HANDLE copyFenceEvent;
        UINT64 copyFenceValue;

        View sceneView;
        View postView;

    public:
    
        Resolution displayResolution;
        Resolution sceneResolution;
    };
}
