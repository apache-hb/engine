#include "game/render.h"
#include "game/gdk.h"

#include "simcoe/core/io.h"

#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "imgui/backends/imgui_impl_win32.h"
#include "imgui/backends/imgui_impl_dx12.h"

#include "imnodes/imnodes.h"

using namespace simcoe;
using namespace game;

ShaderBlob game::loadShader(std::string_view path) {
    std::unique_ptr<Io> file{Io::open(path, Io::eRead)};
    return file->read<std::byte>();
}

D3D12_SHADER_BYTECODE game::getShader(const ShaderBlob &blob) {
    return D3D12_SHADER_BYTECODE {blob.data(), blob.size()};
}

Display game::createDisplay(const system::Size& size) {
    auto [width, height] = size;

    Display display = {
        .viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, float(width), float(height)),
        .scissor = CD3DX12_RECT(0, 0, LONG(width), LONG(height))
    };

    return display;
}

Display game::createLetterBoxDisplay(const system::Size& render, const system::Size& present) {
    auto [renderWidth, renderHeight] = render;
    auto [presentWidth, presentHeight] = present;

    auto widthRatio = float(presentWidth) / renderWidth;
    auto heightRatio = float(presentHeight) / renderHeight;

    float x = 1.f;
    float y = 1.f;

    if (widthRatio < heightRatio) {
        x = widthRatio / heightRatio;
    } else {
        y = heightRatio / widthRatio;
    }

    D3D12_VIEWPORT viewport = CD3DX12_VIEWPORT(
        presentWidth * (1.f - x) / 2.f,
        presentHeight * (1.f - y) / 2.f,
        x * presentWidth,
        y * presentHeight
    );

    D3D12_RECT scissor = CD3DX12_RECT(
        LONG(viewport.TopLeftX),
        LONG(viewport.TopLeftY),
        LONG(viewport.TopLeftX + viewport.Width),
        LONG(viewport.TopLeftY + viewport.Height)
    );

    return { viewport, scissor };
}

/// edges

RenderEdge::RenderEdge(const GraphObject& self, render::Pass *pPass)
    : OutEdge(self, pPass)
{ }

ID3D12Resource *RenderEdge::getResource() {
    auto& data = frameData[getContext().getCurrentFrame()];
    return data.pRenderTarget;
}

D3D12_RESOURCE_STATES RenderEdge::getState() const {
    return D3D12_RESOURCE_STATE_PRESENT;
}

D3D12_CPU_DESCRIPTOR_HANDLE RenderEdge::cpuHandle() {
    auto& ctx = getContext();

    auto& data = frameData[ctx.getCurrentFrame()];
    return ctx.getRtvHeap().cpuHandle(data.index);
}

D3D12_GPU_DESCRIPTOR_HANDLE RenderEdge::gpuHandle() {
    auto& ctx = getContext();
    
    auto& data = frameData[ctx.getCurrentFrame()];
    return ctx.getRtvHeap().gpuHandle(data.index);
}

void RenderEdge::init() {
    auto& ctx = getContext();
    size_t frames = ctx.getFrames();
    auto *pDevice = ctx.getDevice();
    auto *pSwapChain = ctx.getSwapChain();
    auto& rtvHeap = ctx.getRtvHeap();

    frameData.resize(frames);

    for (UINT i = 0; i < frames; i++) {
        auto index = rtvHeap.alloc();
        ID3D12Resource *pRenderTarget = nullptr;
        HR_CHECK(pSwapChain->GetBuffer(i, IID_PPV_ARGS(&pRenderTarget)));
        pDevice->CreateRenderTargetView(pRenderTarget, nullptr, rtvHeap.cpuHandle(index));

        frameData[i] = { .index = index, .pRenderTarget = pRenderTarget };
    }
}

void RenderEdge::deinit() {
    auto& rtvHeap = getContext().getRtvHeap();

    for (auto &frame : frameData) {
        RELEASE(frame.pRenderTarget);
        rtvHeap.release(frame.index);
    }
}

SourceEdge::SourceEdge(const GraphObject& self, render::Pass *pPass, D3D12_RESOURCE_STATES state)
    : SourceEdge(self, pPass, state, nullptr, { }, { })
{ }

SourceEdge::SourceEdge(const GraphObject& self, render::Pass *pPass, D3D12_RESOURCE_STATES state, ID3D12Resource *pResource, D3D12_CPU_DESCRIPTOR_HANDLE cpu, D3D12_GPU_DESCRIPTOR_HANDLE gpu)
    : OutEdge(self, pPass)
    , state(state)
    , pResource(pResource)
    , cpu(cpu)
    , gpu(gpu)
{ }

void SourceEdge::setResource(ID3D12Resource *pOther, D3D12_CPU_DESCRIPTOR_HANDLE otherCpu, D3D12_GPU_DESCRIPTOR_HANDLE otherGpu) {
    pResource = pOther;
    cpu = otherCpu;
    gpu = otherGpu;
}

ID3D12Resource *SourceEdge::getResource() { return pResource; }
D3D12_RESOURCE_STATES SourceEdge::getState() const { return state; }
D3D12_CPU_DESCRIPTOR_HANDLE SourceEdge::cpuHandle() { return cpu; }
D3D12_GPU_DESCRIPTOR_HANDLE SourceEdge::gpuHandle() { return gpu; }

/// passes

ImGuiPass::ImGuiPass(const GraphObject& object, Info& info, input::Manager& manager): Pass(object), info(info), inputManager(manager) { 
    pRenderTargetIn = in<render::InEdge>("render-target", D3D12_RESOURCE_STATE_RENDER_TARGET);
    pRenderTargetOut = out<render::RelayEdge>("render-target", pRenderTargetIn);
}

void ImGuiPass::start() {
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

void ImGuiPass::stop() {
    ImNodes::DestroyContext();

    ImGui_ImplDX12_Shutdown();
    getContext().getCbvHeap().release(fontHandle);
}

void ImGuiPass::execute() {
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    enableDock();
    drawInfo();
    drawRenderGraphInfo();
    drawGdkInfo();
    drawInputInfo();

    ImGui::ShowDemoWindow();
    
    ImGui::Render();
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), getContext().getCommandList());
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
        ImGui::Text("Editor");
        ImGui::Separator();

        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Import")) {

            }

            ImGui::MenuItem("Save");
            ImGui::MenuItem("Open");

            ImGui::EndMenu();
        }

        // align the close button to the right of the window
        auto& style = ImGui::GetStyle();
        ImVec2 closeButtonPos(ImGui::GetWindowWidth() - (style.FramePadding.x * 2) - ImGui::GetFontSize(), 0.f);

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
    }
    ImGui::End();
}

void ImGuiPass::drawRenderGraphInfo() {
    if (ImGui::Begin("Render Graph")) {
        ImNodes::BeginNodeEditor();

        for (auto& [name, pass] : getGraph().getPasses()) {
            ImNodes::BeginNode(passIndices[pass.get()]);

            ImNodes::BeginNodeTitleBar();
            ImGui::Text("%s", name);
            ImNodes::EndNodeTitleBar();

            for (auto& output : pass->getOutputs()) {
                ImNodes::BeginOutputAttribute(edgeIndices[output.get()]);
                ImGui::Text("%s", output->getName());
                ImNodes::EndOutputAttribute();
            }

            for (auto& input : pass->getInputs()) {
                ImNodes::BeginInputAttribute(edgeIndices[input.get()]);
                ImGui::Text("%s", input->getName());
                ImNodes::EndInputAttribute();
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

void ImGuiPass::drawGdkInfo() {
    const auto& analytics = gdk::getAnalyticsInfo();
    
    if (ImGui::Begin("GDK")) {
        ImGui::Text("Family: %s", analytics.family);
        ImGui::Text("Model: %s", analytics.form);

        auto [major, minor, build, revision] = analytics.osVersion;
        ImGui::Text("OS: %u.%u.%u.%u", major, minor, build, revision);
        ImGui::Text("ID: %s", gdk::getConsoleId());

        if (ImGui::BeginTable("features", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY)) {
            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableSetupColumn("Feature", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn("Available", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableHeadersRow();
            for (const auto& [feature, detail] : gdk::getFeatures()) {
                const auto& [name, available] = detail;

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("%s", name);
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%s", available ? "enabled" : "disabled");
            }
            ImGui::EndTable();
        }
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
