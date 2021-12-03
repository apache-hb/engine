#pragma once

#include "util.h"
#include "system/window.h"

namespace engine::render {
    struct Context;

    namespace nodes {
        struct Node {
            using Index = size_t;

            Node(Context *ctx, Index index) 
                : ctx(ctx)
                , index(index)
            { }

            virtual ~Node() { destroy(); }
            
            virtual void render() { }
            virtual void create() = 0;
            virtual void destroy() = 0;

        private:
            friend Context;

            std::vector<Index> depends; /// all nodes that this node depends on
            Index index; /// the index of this node
            Context *ctx; /// the context this node is created from
        };
    }

    struct Context {
        Context(win32::WindowHandle *window = nullptr, UINT frames = 2);
        ~Context();

        void present(nodes::Node *root);

        Factory getFactory() { return factory; }
        std::span<Adapter> adapters() { return factory.adapters(); }
        Adapter currentAdapter() { return adapters()[adapterIndex]; }

        void setWindow(win32::WindowHandle *handle);
        void setFrameBuffering(UINT frames);
        void selectAdapter(size_t index);

    private:
        /// root factory
        Factory factory;
        size_t adapterIndex = SIZE_MAX;

        win32::WindowHandle *window;
        UINT frameCount;

        void createDevice();
        void destroyDevice();

        struct FrameData {
            d3d12::Resource renderTarget;
            d3d12::CommandAllocator commandAllocator;
        };

        /// our current logical device
        d3d12::Device1 device;
        d3d12::CommandQueue commandQueue;
        dxgi::SwapChain3 swapchain;
        UINT currentFrame;

        d3d12::DescriptorHeap rtvHeap;
        UINT rtvHeapIncrement;

        FrameData *frameData;

        /// our rendergraph
        std::vector<nodes::Node*> data;
    };
}
