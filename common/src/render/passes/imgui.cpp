#include "engine/render/passes/imgui.h"

#include "imgui/imgui.h"

using namespace simcoe;
using namespace simcoe::render;

ImGuiPass::ImGuiPass(const Info& info, ImGuiUpdate update) 
    : Pass(info, CommandSlot::ePost) 
    , update(update) 
{
    renderTargetIn = newInput<Input>("rtv", State::eRenderTarget);
    renderTargetOut = newOutput<Relay>("rtv", renderTargetIn);
}

void ImGuiPass::init() {
    auto& ctx = getContext();
    imguiHeapOffset = ctx.getHeap().alloc(DescriptorSlot::eImGui);
    ctx.imguiInit(imguiHeapOffset);
}

void ImGuiPass::deinit() {
    auto& ctx = getContext();
    ctx.imguiShutdown();
    ctx.getHeap().release(DescriptorSlot::eImGui, imguiHeapOffset);
}

void ImGuiPass::execute() {
    auto& ctx = getContext();
    auto& cmd = getCommands();
    // imgui work

    ctx.imguiFrame();
    ImGui::NewFrame();

    update();

    for (auto& [name, pass] : getParent()->passes) {
        pass->imgui();
    }

    cmd.imguiFrame();
}
