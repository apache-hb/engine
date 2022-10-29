#pragma once

#include <map>
#include <string>
#include <vector>

#include "engine/memory/bitmap.h"
#include "engine/rhi/rhi.h"

namespace simcoe::graph {
    struct RenderGraph;

    struct Info {
        RenderGraph *parent;
        size_t index;
        std::string name;
    };

    struct RenderResource {
        RenderResource(const Info& create) : info(create) { }

    private:
        Info info;
    };

    struct RenderPass {
        RenderPass(const Info &create) : info(create) { }

        std::map<std::string, RenderResource*> inputs;
        std::map<std::string, RenderResource*> outputs;

    private:
        Info info;
    };

    struct RenderGraph {
        struct Create {
            Window *window;
            rhi::TextureSize resolution = { 1280, 720 }; // default resolution
            size_t backBuffers = 2;

            // tuning constants
            size_t heapSize = 1024; // size of the descriptor heap
        };

        /**
         * @brief Construct a new Render Graph object
         * 
         * @param window the window to attach to
         * @param resolution the internal render resolution
         */
        RenderGraph(const Create& create);

        template<typename T, typename... A> requires (std::is_base_of_v<RenderPass, T>)
        T *newPass(const std::string& name, A&&... args) {
            Info init = { this, passes.size(), name };
            auto pass = new T(init, std::forward<A>(args)...);
            passes.push_back(pass);
            return pass;
        }

        template<typename T, typename... A> requires (std::is_base_of_v<RenderResource, T>)
        T *newResource(const std::string& name, A&&... args) {
            Info init = { this, passes.size(), name };
            auto resource = new T(init, std::forward<A>(args)...);
            resources.push_back(resource);
            return resource;
        }

        void update(const Create& create);

    private:
        Create info;

        void createBackBuffers();
        void createIntermediateTarget();

        rhi::Device device;

        // presentation data
        rhi::CommandQueue queue;
        rhi::SwapChain swapchain;

        // backbuffers
        rhi::DescriptorSet rtvHeap;

        // per frame data
        UniquePtr<rhi::Buffer[]> backBuffers;
        UniquePtr<rhi::Allocator[]> allocators;

        // current backbuffer index
        size_t currentFrame = 0;

        rhi::Buffer intermediateTarget;
        rhi::View view;

        // data buffers
        rhi::DescriptorSet cbvHeap;
        memory::BitMap cbvAlloc;

        // render graph data
        std::vector<RenderPass*> passes;
        std::vector<RenderResource*> resources;

        // allocated indices
        size_t intermediateSlot = SIZE_MAX;
        size_t imguiSlot = SIZE_MAX;

        math::Resolution<int> windowSize() const { return info.window->size(); }

        // +1 for the intermediate target. store the intermediate target in the first slot
        // so we dont need to recreate it when the number of backbuffers changes
        //
        // 0 | 1..N
        // ^  ^~~~~ backbuffers
        // intermediate target
        constexpr size_t totalRtvSlots() const { return info.backBuffers + 1; }
        constexpr size_t getRtvSlot(size_t index) const { return index + 1; }
        constexpr size_t intermediateRtvSlot() const { return 0; }
    };
}
