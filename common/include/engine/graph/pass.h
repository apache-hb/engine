#pragma once

#include <unordered_map>

#include "engine/graph/graph.h"
#include "engine/graph/bind.h"
#include "engine/graph/resource.h"

namespace simcoe::graph {
    struct RenderPass {
        RenderPass(const Info &create) : info(create) { 
            init();
        }

        virtual ~RenderPass() = default;

        virtual void init() { }
        virtual void execute() = 0;

        const char *getName() const { return info.name; }

        Input *getInput(const char *name) { return inputs[name].get(); }
        Output *getOutput(const char *name) { return outputs[name].get(); }

    protected:
        RenderGraph *getParent() { return info.parent; }
        Info info;

        std::unordered_map<const char*, UniquePtr<Input>> inputs;
        std::unordered_map<const char*, UniquePtr<Output>> outputs;
    };

    struct CommandPass : RenderPass {
        CommandPass(const Info &create) : RenderPass(create) { 
            inputs["device"] = new Input(&device);
            outputs["commands"] = new Output(&commands);
        }

        void execute() override { 
            
        }

    protected:
        rhi::CommandList &cmd() { return commands->get(); }
        DeviceResource *device = nullptr;
        CommandResource *commands = nullptr;
    };

    struct DirectPass : CommandPass {
        void init() override;
    };

    struct SetupPass final : RenderPass {
        SetupPass(const Info &create) : RenderPass(create) { 
            outputs["device"] = new Output(&device);
            outputs["swapchain"] = new Output(&swapchain);
        }

        void execute() override { }

    private:
        DeviceResource *device = nullptr;
        SwapChainResource *swapchain = nullptr;
    };

    struct ImGuiPass final : CommandPass {
        ImGuiPass(const Info &create) : CommandPass(create) { }

        void execute() override {

        }
    };

    struct PresentPass final : RenderPass {
        PresentPass(const Info &create) : RenderPass(create) { 
            inputs["backbuffer"] = new Input(&intermediate, rhi::Buffer::State::eRenderTarget);
            inputs["swapchain"] = new Input(&swapchain, rhi::Buffer::State::eRenderTarget);
        }

        void execute() override {
            swapchain->getSwapChain().present();
        }

    private:
        TextureResource *intermediate;
        SwapChainResource *swapchain;
    };

    struct ClearPass final : CommandPass {
        ClearPass(const Info &create, math::float4 clear) : CommandPass(create), clear(clear) { 
            inputs["buffer"] = new Input(&buffer, rhi::Buffer::State::eRenderTarget);
            outputs["buffer"] = new Output(&buffer, rhi::Buffer::State::eRenderTarget);
        }

        void execute() override {
            cmd().clearRenderTarget(buffer->getCpuHandle(), clear);
        }

    private:
        math::float4 clear;

        TextureResource *buffer;
    };
}
