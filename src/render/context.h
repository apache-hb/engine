#pragma once

#include <functional>

#include "util/macros.h"
#include "system/system.h"
#include "objects/factory.h"
#include "atomic-queue/queue.h"

namespace engine::render {
    struct Context;
    
    constexpr float kClearColour[] = { 0.0f, 0.2f, 0.4f, 1.0f };

    namespace Allocators {
        enum Index : size_t {
            Copy, /// allocator used for copy commands
            ImGui, /// allocator used for imgui commands
            Total
        };
    }

    namespace Targets {
        enum Index : size_t {
            Scene,
            Total
        };
    }

    namespace Queues {
        enum Index : size_t {
            Direct,
            Copy,
            Total
        };

        constexpr CommandList::Type type(Index index) {
            switch (index) {
            case Direct: return CommandList::kDirect;
            case Copy: return CommandList::kCopy;
            default: UNREACHABLE;
            }
        }

        constexpr std::string_view name(Index index) {
            switch (index) {
            case Direct: return "direct-command-queue";
            case Copy: return "copy-command-queue";
            default: UNREACHABLE;
            }
        }
    }

    struct Context {
        using Event = std::function<void()>;

        struct FrameData {
            Fence::Value value = 0;
            Resource target;
        };

        struct Budget {
            size_t queueSize;
        };

        struct Create {
            // render device settings
            Factory* factory;
            size_t adapter;
            
            // render output settings
            system::Window* window;
            Resolution resolution;

            // render config settings
            UINT buffers;
            bool vsync;

            Budget budget;
        };

        Context(Create&& create);

        // external api
        void create();
        void destroy();

        void present();

        void addEvent(Event&& event);

        // binding api
        void bindRtv(Resource resource, DescriptorHeap::CpuHandle handle);
        void bindSrv(Resource resource, DescriptorHeap::CpuHandle handle);
        void bindCbv(Resource resource, UINT size, DescriptorHeap::CpuHandle handle);

    private:
        // api agnostic data and creation data
        Create info;
        ubn::queue<Event> events;

        Factory& getFactory();
        Adapter& getAdapter();
        system::Window& getWindow();

        DXGI_FORMAT getFormat();
        UINT getBufferCount();

        Resolution getSceneResolution();
        Resolution getPostResolution();

        // d3d12 api objects
        void createDevice();
        void destroyDevice();

        void createCommandQueues();
        void destroyCommandQueues();

        void createSwapChain();
        void destroySwapChain();

        void createRtvHeap();
        void destroyRtvHeap();

        void createCbvHeap();
        void destroyCbvHeap();

        void createFrameData();
        void destroyFrameData();

        void createFence();
        void destroyFence();

        Device device;
        CommandQueue queues[Queues::Total];
        SwapChain3 swapchain;
        UINT frameIndex;

        // our our render targets including our scene target
        // layed out such that
        // [0] = scene target
        // [1..N] = back buffers
        DescriptorHeap rtvHeap;
    
        // our constant buffer views
        // layout derived from Targets::Index
        DescriptorHeap cbvHeap;

        DescriptorHeap::CpuHandle rtvCpuHandle(UINT index);

        Resource sceneTarget;
        FrameData* frameData;

        // sync objects and methods
        Fence fence;
        HANDLE fenceEvent;

        void waitForGpu();
        void nextFrame();

        // display managment

        void createViews();

        // our internal resolution
        // by extension this is the resolution of our intermediate target
        View sceneView;
        Resolution sceneResolution;

        // our extern resolution
        // by extension this is the resolution of our swapchain
        // this should also be the resolution of the actual window
        View postView;
        Resolution postResolution;
    };
}
