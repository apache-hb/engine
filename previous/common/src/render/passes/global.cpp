#include "engine/render/passes/global.h"

#include "imgui/imgui.h"
#include "imnodes/imnodes.h"

using namespace simcoe;
using namespace simcoe::render;

GlobalPass::GlobalPass(const GraphObject& info) : Pass(info, CommandSlot::ePost) {
    rtvResource = getParent().addResource<RenderTargetResource>("rtv");

    renderTargetOut = newOutput<Source>("rtv", State::ePresent, rtvResource);
}

void GlobalPass::init() {
    ImNodes::CreateContext();

    auto& passes = getParent().passes;
    int i = 0;
    for (auto& [name, pass] : passes) {
        passIds[pass.get()] = i++;
        for (auto& input : pass->inputs) {
            wireIds[input.get()] = i++;
        }

        for (auto& output : pass->outputs) {
            wireIds[output.get()] = i++;
        }
    }

    int l = 0;
    for (auto& [source, dst] : getParent().wires) {
        auto from = wireIds[source];
        auto to = wireIds[dst];

        links[l++] = { from, to };
    }
}

void GlobalPass::deinit() {
    ImNodes::DestroyContext();
}

void GlobalPass::imgui() {
    auto& ctx = getContext();
    if (ImGui::Begin("Context state")) {
        ImGui::Text("Total frames: %zu", ctx.getFrames());

        auto [wx, wy] = ctx.windowSize();
        ImGui::Text("Present resolution: %zu x %zu", wx, wy);

        auto [sx, sy] = ctx.sceneSize();
        ImGui::Text("Scene resolution: %zu x %zu", sx, sy);

        auto& heap = ctx.getHeap();
        ImGui::Text("CBV Heap: %zu/%zu", heap.getUsed(), heap.getSize());

        if (ImGui::BeginTable("Slots", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoHostExtendX)) {
            ImGui::TableSetupColumn("Index");
            ImGui::TableSetupColumn("Type");
            ImGui::TableHeadersRow();

            for (size_t i = 0; i < heap.getUsed(); i++) {
                ImGui::TableNextRow();

                ImGui::TableNextColumn();
                ImGui::Text("%zu", i);

                ImGui::TableNextColumn();
                ImGui::Text("%s", getSlotName(heap.getBit(i)));
            }

            ImGui::EndTable();
        }
    }

    ImGui::End();

    if (ImGui::Begin("Graph state")) {
        ImNodes::BeginNodeEditor();

        for (auto& [name, pass] : getParent().passes) {
            ImNodes::BeginNode(passIds[pass.get()]);
            
            ImNodes::BeginNodeTitleBar();
            ImGui::TextUnformatted(name);
            ImNodes::EndNodeTitleBar();

            for (auto& input : pass->inputs) {
                ImNodes::BeginInputAttribute(wireIds[input.get()]);
                ImGui::TextUnformatted(input->getName());
                ImNodes::EndInputAttribute();
            }

            for (auto& output : pass->outputs) {
                ImNodes::BeginOutputAttribute(wireIds[output.get()]);
                ImGui::TextUnformatted(output->getName());
                ImNodes::EndOutputAttribute();
            }

            ImNodes::EndNode();
        }

        for (auto& [id, link] : links) {
            auto [from, to] = link;
            ImNodes::Link(id, from, to);
        }

        ImNodes::EndNodeEditor();
    }

    ImGui::End();
}