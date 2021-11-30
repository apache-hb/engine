#pragma once

#include "util.h"

namespace render {
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
            std::vector<Index> depends; /// all nodes that this node depends on
            Index index; /// the index of this node
            Context *ctx; /// the context this node is created from
        };

#if 0
        struct DeviceNode : Node {
            virtual void create() {
                auto adapter = ctx->currentAdapter();
                ensure(D3D12CreateDevice(adapter.get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device)), "create-device");
            }

            virtual void destroy() {
                device.drop();
            }

        private:
            d3d12::Device1 device;
        };

        struct RootNode : Node {
            virtual void create() { }
            virtual void destroy() { }

            virtual void render() {
                auto swapchain = ctx->getSwapchain();
                ensure(swapchain->Present(1, 0), "swapchain-present");
            }
        };
#endif
    }

    struct Context {
        Context();
        ~Context();

        void present(nodes::Node *root);

        Factory getFactory() { return factory; }
        std::vector<Adapter> adapters() { return factory.adapters(); }
    
    private:
        Factory factory;
        std::vector<nodes::Node*> nodes;
    };
}
