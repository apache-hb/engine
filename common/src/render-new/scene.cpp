#include "engine/render-new/scene.h"

#include "engine/rhi/rhi.h"
#include "imgui/imgui.h"

#include <array>

using namespace simcoe;
using namespace simcoe::render;

struct PresentPass final : Pass {
    using Pass::Pass;

    void execute(Context& ctx) override {
        ctx.present();
    }
};

struct ImGuiPass final : Pass {
    using Pass::Pass;
    
    constexpr static math::float4 kLetterBox = { 0.f, 0.f, 0.f, 1.f };

    void init(Context& ctx) override {
        auto& device = ctx.getDevice();

        auto [width, height] = ctx.windowSize();

        view = { 0.f, 0.f, float(width), float(height) };

        allocators = new rhi::Allocator[ctx.getFrames()];
        for (size_t i = 0; i < ctx.getFrames(); i++) {
            allocators[i] = device.newAllocator(rhi::CommandList::Type::eDirect);
        }
        commands = device.newCommandList(allocators[ctx.currentFrame()], rhi::CommandList::Type::eDirect);

        imguiHeapOffset = ctx.getHeap().alloc(DescriptorSlot::eImGui);
        ctx.imguiInit(imguiHeapOffset);
    }

    void deinit(Context& ctx) override {
        ctx.imguiShutdown();
        ctx.getHeap().release(DescriptorSlot::eImGui, imguiHeapOffset);
    }

    void execute(Context& ctx) override {
        auto kDescriptors = std::to_array({ ctx.getHeap().getHeap().get() });
        auto kBeginTransition = std::to_array({
            rhi::newStateTransition(ctx.getRenderTarget(), rhi::Buffer::State::ePresent, rhi::Buffer::State::eRenderTarget)
        });

        auto kEndTransition = std::to_array({
            rhi::newStateTransition(ctx.getRenderTarget(), rhi::Buffer::State::eRenderTarget, rhi::Buffer::State::ePresent)
        });

        commands.beginRecording(allocators[ctx.currentFrame()]);
        commands.transition(kBeginTransition);
        commands.bindDescriptors(kDescriptors);
        commands.setRenderTarget(ctx.getRenderTargetHandle(), rhi::CpuHandle::Invalid, kLetterBox);
        commands.setViewAndScissor(view);

        // imgui work

        ctx.imguiFrame();
        ImGui::NewFrame();

        ImGui::SetNextFrameWantCaptureKeyboard(true);
        ImGui::SetNextFrameWantCaptureMouse(true);
        
        ImGui::ShowDemoWindow();
        commands.imguiFrame();

        commands.transition(kEndTransition);
        commands.endRecording();

        ctx.submit(commands.get());
    }

    rhi::View view;

    UniquePtr<rhi::Allocator[]> allocators;
    rhi::CommandList commands;

    size_t imguiHeapOffset;
};

WorldGraph::WorldGraph() {
    auto imgui = addPass<ImGuiPass>("imgui");
    auto present = addPass<PresentPass>("present");

    link(imgui, present);

    primary = present;
}
