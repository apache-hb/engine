#include "imnodes/include/imnodes/imnodes.h"
#include "simcoe/core/system.h"
#include "simcoe/core/win32.h"

#include "simcoe/input/desktop.h"
#include "simcoe/input/gamepad.h"

#include "simcoe/locale/locale.h"
#include "simcoe/audio/audio.h"
#include "simcoe/render/context.h"
#include "simcoe/render/graph.h"

#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_win32.h"
#include "imgui/backends/imgui_impl_dx12.h"

#include "imnodes/imnodes.h"

#include "simcoe/simcoe.h"

#include "GameInput.h"
#include "XGameRuntime.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

using namespace simcoe;

#if 0
struct Feature {
    XGameRuntimeFeature feature;
    const char *name;
};

static inline constexpr auto kFeatures = std::to_array<Feature>({
    { XGameRuntimeFeature::XAccessibility, "XAccessibility" },
    { XGameRuntimeFeature::XAppCapture, "XAppCapture" },
    { XGameRuntimeFeature::XAsync, "XAsync" },
    { XGameRuntimeFeature::XAsyncProvider, "XAsyncProvider" },
    { XGameRuntimeFeature::XDisplay, "XDisplay" },
    { XGameRuntimeFeature::XGame, "XGame" },
    { XGameRuntimeFeature::XGameInvite, "XGameInvite" },
    { XGameRuntimeFeature::XGameSave, "XGameSave" },
    { XGameRuntimeFeature::XGameUI, "XGameUI" },
    { XGameRuntimeFeature::XLauncher, "XLauncher" },
    { XGameRuntimeFeature::XNetworking, "XNetworking" },
    { XGameRuntimeFeature::XPackage, "XPackage" },
    { XGameRuntimeFeature::XPersistentLocalStorage, "XPersistentLocalStorage" },
    { XGameRuntimeFeature::XSpeechSynthesizer, "XSpeechSynthesizer" },
    { XGameRuntimeFeature::XStore, "XStore" },
    { XGameRuntimeFeature::XSystem, "XSystem" },
    { XGameRuntimeFeature::XTaskQueue, "XTaskQueue" },
    { XGameRuntimeFeature::XThread, "XThread" },
    { XGameRuntimeFeature::XUser, "XUser" },
    { XGameRuntimeFeature::XError, "XError" },
    { XGameRuntimeFeature::XGameEvent, "XGameEvent" },
    { XGameRuntimeFeature::XGameStreaming, "XGameStreaming" },
});

struct Features {
    Features() {
        size_t size = 0;
        HR_CHECK(XSystemGetConsoleId(sizeof(id), id, &size));

        info = XSystemGetAnalyticsInfo();

        for (size_t i = 0; i < kFeatures.size(); ++i) {
            features[i] = XGameRuntimeIsFeatureAvailable(kFeatures[i].feature);
        }
    }
    XSystemAnalyticsInfo info;
    bool features[kFeatures.size()] = {};
    char id[128] = {};
};

struct XGameRuntime {
    XGameRuntime() {
        HR_CHECK(XGameRuntimeInitialize());
    }

    ~XGameRuntime() {
        XGameRuntimeUninitialize();
    }
};
#endif

struct Info {
    Info() :gamepad(0), keyboard(), mouse(false, true) {
        manager.add(&gamepad);
        manager.add(&keyboard);
        manager.add(&mouse);
    }

    void poll() {
        manager.poll();
    }

    Locale locale;

    input::Gamepad gamepad;
    input::Keyboard keyboard;
    input::Mouse mouse;

    input::Manager manager;
};

struct Window : system::Window {
    Window(Info& info)
        : system::Window("game", { 1280, 720 })
        , info(info) 
    { }

    bool poll() {
        info.mouse.update(getHandle());
        return system::Window::poll();
    }

    LRESULT onEvent(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) override {
        info.keyboard.update(msg, wparam, lparam);

        return ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam);
    }

    Info& info;
};

struct GlobalPass final : render::Pass {
    GlobalPass(const GraphObject& object, const math::size2& size) : Pass(object) { 
        auto [width, height] = size;

        viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height));
        scissor = CD3DX12_RECT(0, 0, static_cast<LONG>(width), static_cast<LONG>(height));
    
        pRenderTargetOut = out<render::RenderEdge>("render-target");
    }

    void execute() override {
        auto& ctx = getContext();
        auto cmd = ctx.getCommandList();
        auto rtv = pRenderTargetOut->cpuHandle();

        ASSERTF(rtv.ptr != 0, "No render target set!");

        ctx.begin();

        cmd->RSSetViewports(1, &viewport);
        cmd->RSSetScissorRects(1, &scissor);

        cmd->OMSetRenderTargets(1, &rtv, false, nullptr);
    }

    render::OutEdge *pRenderTargetOut = nullptr;

private:
    D3D12_VIEWPORT viewport;
    D3D12_RECT scissor;
};

struct RenderClearPass final : render::Pass {
    RenderClearPass(const GraphObject& object) : Pass(object) {
        pTargetIn = in<render::InEdge>("render-target", D3D12_RESOURCE_STATE_RENDER_TARGET);
        pTargetOut = out<render::RelayEdge>("render-target", pTargetIn);
    }

    void execute() override {
        auto& ctx = getContext();
        auto cmd = ctx.getCommandList();
        auto rtv = pTargetIn->cpuHandle();

        cmd->ClearRenderTargetView(rtv, colour, 0, nullptr);
    }

    render::InEdge *pTargetIn = nullptr;
    render::OutEdge *pTargetOut = nullptr;

private:
    float colour[4] = { 0.0f, 0.2f, 0.4f, 1.0f };
};

struct PresentPass final : render::Pass {
    PresentPass(const GraphObject& object) : Pass(object) { 
        pRenderTargetIn = in<render::InEdge>("render-target", D3D12_RESOURCE_STATE_PRESENT);
    }

    void start() override { }
    void stop() override { }

    void execute() override {
        auto& ctx = getContext();
        ctx.end();
    }

    render::InEdge *pRenderTargetIn = nullptr;
};

struct ImGuiPass final : render::Pass {
    ImGuiPass(const GraphObject& object, Info& info): Pass(object), info(info) { 
        pRenderTargetIn = in<render::InEdge>("render-target", D3D12_RESOURCE_STATE_RENDER_TARGET);
        pRenderTargetOut = out<render::RelayEdge>("render-target", pRenderTargetIn);
    }
    
    void start() override {
        auto& context = getContext();
        auto& heap = context.getHeap();
        fontHandle = heap.alloc();

        ImGui_ImplDX12_Init(
            context.getDevice(),
            int(context.getFrames()),
            DXGI_FORMAT_R8G8B8A8_UNORM,
            heap.getHeap(),
            heap.cpuHandle(fontHandle),
            heap.gpuHandle(fontHandle)
        );

        ImNodes::CreateContext();

        int id = 0;
        for (auto& [name, pass] : getGraph().getPasses()) {
            passIndices[pass.get()] = id++;
            for (auto& input : pass->getInputs()) {
                edgeIndices[input.get()] = id++;
            }

            for (auto& output : pass->getOutputs()) {
                edgeIndices[output.get()] = id++;
            }
        }

        int link = 0;
        for (auto& [src, dst] : getGraph().getEdges()) {
            int srcId = edgeIndices[src];
            int dstId = edgeIndices[dst];

            links[link++] = { srcId, dstId };
        }
    }

    void stop() override {
        ImNodes::DestroyContext();

        ImGui_ImplDX12_Shutdown();
        getContext().getHeap().release(fontHandle);
    }

    void execute() override {
        ImGui_ImplDX12_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        enableDock();
        drawRenderGraphInfo();
        // drawGdkInfo();
        drawInputInfo();

        ImGui::ShowDemoWindow();
        
        ImGui::Render();
        ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), getContext().getCommandList());
    }

    render::InEdge *pRenderTargetIn = nullptr;
    render::OutEdge *pRenderTargetOut = nullptr;
private:
    Info& info;
    render::Heap::Index fontHandle = render::Heap::Index::eInvalid;

    void enableDock() {
        constexpr auto kDockFlags = ImGuiDockNodeFlags_PassthruCentralNode;

        constexpr auto kWindowFlags = 
            ImGuiWindowFlags_MenuBar |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoBackground |
            ImGuiWindowFlags_NoBringToFrontOnFocus | 
            ImGuiWindowFlags_NoNavFocus;
            
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
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("Import")) {

                }
                ImGui::MenuItem("Save");

                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }

        ImGui::End();
    }

    void drawRenderGraphInfo() {
        if (ImGui::Begin("Render Graph")) {
            ImNodes::BeginNodeEditor();

            for (auto& [name, pass] : getGraph().getPasses()) {
                ImNodes::BeginNode(passIndices[pass.get()]);

                ImNodes::BeginNodeTitleBar();
                ImGui::Text("%s", name);
                ImNodes::EndNodeTitleBar();

                for (auto& input : pass->getInputs()) {
                    ImNodes::BeginInputAttribute(edgeIndices[input.get()]);
                    ImGui::Text("%s", input->getName());
                    ImNodes::EndInputAttribute();
                }

                for (auto& output : pass->getOutputs()) {
                    ImNodes::BeginOutputAttribute(edgeIndices[output.get()]);
                    ImGui::Text("%s", output->getName());
                    ImNodes::EndOutputAttribute();
                }

                ImNodes::EndNode();
            }

            for (auto& [id, link] : links) {
                ImNodes::Link(id, link.src, link.dst);
            }

            ImNodes::EndNodeEditor();
        }
        ImGui::End();
    }

#if 0
    void drawGdkInfo() {
        if (ImGui::Begin("GDK")) {
            const auto& features = info.features;
            ImGui::Text("Family: %s", features.info.family);
            ImGui::SameLine();

            auto [major, minor, build, revision] = features.info.osVersion;
            ImGui::Text("OS: %u.%u.%u.%u", major, minor, build, revision);
            ImGui::Text("ID: %s", features.id);

            if (ImGui::BeginTable("features", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY)) {
                ImGui::TableSetupScrollFreeze(0, 1);
                ImGui::TableSetupColumn("Feature", ImGuiTableColumnFlags_WidthFixed);
                ImGui::TableSetupColumn("Available", ImGuiTableColumnFlags_WidthFixed);
                ImGui::TableHeadersRow();
                for (size_t i = 0; i < kFeatures.size(); i++) {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("%s", kFeatures[i].name);
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%s", features.features[i] ? "enabled" : "disabled");
                }
                ImGui::EndTable();
            }
        }
        ImGui::End();
    }
#endif

    void drawInputInfo() {
        constexpr auto kTableFlags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY | ImGuiTableFlags_NoHostExtendX;
        const float kTextWidth = ImGui::CalcTextSize("A").x;

        if (ImGui::Begin("Input")) {
            const auto& state = info.manager.getState();
            
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

    struct Link {
        int src;
        int dst;
    };

    std::unordered_map<render::Pass*, int> passIndices;
    std::unordered_map<render::Edge*, int> edgeIndices;
    std::unordered_map<int, Link> links;
};

struct Scene final : render::Graph {
    Scene(render::Context& context, Info& info) : render::Graph(context) {
        pGlobalPass = addPass<GlobalPass>("global", math::size2::from(1280, 720));
        pRenderClearPass = addPass<RenderClearPass>("clear");
        pImGuiPass = addPass<ImGuiPass>("imgui", info);
        pPresentPass = addPass<PresentPass>("present");

        connect(pGlobalPass->pRenderTargetOut, pRenderClearPass->pTargetIn);
        connect(pRenderClearPass->pTargetOut, pImGuiPass->pRenderTargetIn);
        connect(pImGuiPass->pRenderTargetOut, pPresentPass->pRenderTargetIn);
    }

    void execute() {
        Graph::execute(pPresentPass);
    }

private:
    GlobalPass *pGlobalPass;
    RenderClearPass *pRenderClearPass;
    ImGuiPass *pImGuiPass;
    PresentPass *pPresentPass;
};

struct ImGuiRuntime {
    ImGuiRuntime() {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui::StyleColorsDark();

        auto& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    }

    ~ImGuiRuntime() {
        ImGui::DestroyContext();
    }
};

int commonMain() {
    system::System system;
    ImGuiRuntime imgui;
    Info detail;

    Window window { detail };

    render::Context::Info info = {
        .resolution = { 600, 800 }
    };
    render::Context context { window, info };
    Scene scene { context, detail };

    ImGui_ImplWin32_Init(window.getHandle());
    ImGui_ImplWin32_EnableDpiAwareness();

    scene.start();

    while (window.poll()) {
        detail.poll();
        scene.execute();
    }

    scene.stop();

    ImGui_ImplWin32_Shutdown();

    gLog.info("bye bye");

    return 0;
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    return commonMain();
}

int main(int, const char **) {
    return commonMain();
}
