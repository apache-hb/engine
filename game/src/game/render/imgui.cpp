#include "game/render.h"

#include "game/gdk.h"
#include "game/registry.h"

#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "imgui/backends/imgui_impl_win32.h"
#include "imgui/backends/imgui_impl_dx12.h"

#include "imnodes/imnodes.h"

using namespace simcoe;
using namespace game;

namespace {
    constexpr const char *getLevelName(logging::Level level) {
        switch (level) {
        case logging::eInfo: return "info";
        case logging::eWarn: return "warn";
        case logging::eFatal: return "fatal";
        default: return "unknown";
        }
    }
}

ImGuiPass::ImGuiPass(const GraphObject& object, Info& info, input::Manager& manager): Pass(object), info(info), inputManager(manager) { 
    pRenderTargetIn = in<render::InEdge>("render-target", D3D12_RESOURCE_STATE_RENDER_TARGET);
    pRenderTargetOut = out<render::RelayEdge>("render-target", pRenderTargetIn);
}

void ImGuiPass::start(ID3D12GraphicsCommandList*) {
    auto& context = getContext();
    auto& heap = context.getCbvHeap();
    fontHandle = heap.alloc();

    ImGui_ImplDX12_Init(
        context.getDevice(),
        int(context.getFrames()),
        DXGI_FORMAT_R8G8B8A8_UNORM,
        heap.getHeap(),
        heap.cpuHandle(fontHandle),
        heap.gpuHandle(fontHandle)
    );
}

void ImGuiPass::stop() {
    ImNodes::SaveCurrentEditorStateToIniFile("imnodes.ini");
    ImNodes::DestroyContext();

    ImGui_ImplDX12_Shutdown();
    getContext().getCbvHeap().release(fontHandle);
}

void ImGuiPass::execute(ID3D12GraphicsCommandList *pCommands) {
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    enableDock();
    drawInfo();
    drawInputInfo();

    for (auto& entry : game::debug.entries) {
        const auto& extra = game::debug.get(entry);

        if (!extra.enabled) {
            continue;
        }

        if (ImGui::Begin(extra.pzName)) {
            entry->apply();
        }
        ImGui::End();
    }

    fileBrowser.Display();

    if (fileBrowser.HasSelected()) {
        auto path = fileBrowser.GetSelected();
        static_cast<game::Scene&>(getGraph()).load(path);
        fileBrowser.ClearSelected();
    }

    ImGui::ShowDemoWindow();
    
    ImGui::Render();
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), pCommands);
}

void ImGuiPass::enableDock() {
    constexpr auto kDockFlags = ImGuiDockNodeFlags_PassthruCentralNode;

    constexpr auto kWindowFlags = 
        ImGuiWindowFlags_MenuBar |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoBackground |
        ImGuiWindowFlags_NoBringToFrontOnFocus | 
        ImGuiWindowFlags_NoNavFocus |
        ImGuiWindowFlags_NoDocking;
        
    const auto *viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));

    ImGui::Begin("Editor", nullptr, kWindowFlags);

    ImGui::PopStyleVar(3);

    ImGuiID id = ImGui::GetID("EditorDock");
    ImGui::DockSpace(id, ImVec2(0.f, 0.f), kDockFlags);
    
    if (ImGui::BeginMenuBar()) {
        ImGui::Text("Editor");
        ImGui::Separator();

        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Import GLTF")) {
                fileBrowser.SetTitle("GLTF");
                fileBrowser.SetTypeFilters({ ".gltf", ".glb" });
                fileBrowser.Open();
            }

            ImGui::MenuItem("Save");
            ImGui::MenuItem("Open");

            ImGui::EndMenu();
        }

        // align the close button to the right of the window
        auto& style = ImGui::GetStyle();
        ImVec2 closeButtonPos(ImGui::GetWindowWidth() - (style.FramePadding.x * 2) - ImGui::GetFontSize(), 0.f);

        // TODO: icky
        if (ImGui::CloseButton(ImGui::GetID("CloseEditor"), closeButtonPos)) {
            PostQuitMessage(0);
        }

        ImGui::EndMenuBar();
    }

    ImGui::End();
}

void ImGuiPass::drawInfo() {
    if (ImGui::Begin("Info")) {
        auto [displayWidth, displayHeight] = info.windowResolution;
        ImGui::Text("Display: %zu x %zu", displayWidth, displayHeight);

        auto [renderWidth, renderHeight] = info.renderResolution;
        ImGui::Text("Render: %zu x %zu", renderWidth, renderHeight);

        ImGui::Text("GDK: %s", gdk::enabled() ? "Enabled" : "Disabled");

        ImGui::Separator();
        ImGui::Text("Logs");
        
        if (ImGui::BeginChild("Scrolling", ImVec2(0.f, 0.f), true, ImGuiWindowFlags_AlwaysVerticalScrollbar | ImGuiWindowFlags_AlwaysHorizontalScrollbar)) {
            ImGuiListClipper clipper;
            clipper.Begin(int(info.sink.entries.size()));
            while (clipper.Step()) {
                for (int offset = clipper.DisplayStart; offset < clipper.DisplayEnd; offset++) {
                    auto& entry = info.sink.entries[offset];
                    ImGui::Text("[%s] %s", getLevelName(entry.level), entry.message.c_str());
                }
            }
            clipper.End();
        }
        ImGui::EndChild();
    }
    ImGui::End();
}

void ImGuiPass::drawInputInfo() {
    constexpr auto kTableFlags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY | ImGuiTableFlags_NoHostExtendX;
    const float kTextWidth = ImGui::CalcTextSize("A").x;

    if (ImGui::Begin("Input")) {
        const auto& state = inputManager.getState();
        
        ImGui::Text("Current: %s", state.device == input::Device::eGamepad ? "Gamepad" : "Mouse & Keyboard");

        if (ImGui::BeginTable("keys", 2, kTableFlags, ImVec2(kTextWidth * 40, 0.0f))) {
            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableSetupColumn("Key", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn("Index", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableHeadersRow();
            for (size_t i = 0; i < state.key.size(); i++) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("%s", info.locale.get(input::Key(i)));
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%zu", state.key[i]);
            }
            ImGui::EndTable();
        }

        ImGui::SameLine();

        if (ImGui::BeginTable("axis", 2, kTableFlags, ImVec2(kTextWidth * 45, 0.0f))) {
            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableSetupColumn("Axis", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn("Offset", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableHeadersRow();
            for (size_t i = 0; i < state.axis.size(); i++) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("%s", info.locale.get(input::Axis(i)));
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%f", state.axis[i]);
            }
            ImGui::EndTable();
        }
    }
    ImGui::End();
}
