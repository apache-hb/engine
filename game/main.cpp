#include "game/gdk.h"

#include "simcoe/core/system.h"
#include "simcoe/core/io.h"

#include "simcoe/input/desktop.h"
#include "simcoe/input/gamepad.h"

#include "simcoe/locale/locale.h"

#include "simcoe/render/context.h"
#include "simcoe/render/graph.h"

#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "imgui/backends/imgui_impl_win32.h"
#include "imgui/backends/imgui_impl_dx12.h"

#include "imnodes/imnodes.h"

#include "simcoe/simcoe.h"

#include <filesystem>

#include <fastgltf/fastgltf_parser.hpp>
#include <fastgltf/fastgltf_types.hpp>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace gdk = game::gdk;

using namespace simcoe;

namespace {
    using ShaderBlob = std::vector<std::byte>;

    struct Vertex {
        math::float3 position;
        math::float2 uv;
    };

    struct Display {
        D3D12_VIEWPORT viewport;
        D3D12_RECT scissor;
    };

    ShaderBlob loadShader(std::string_view path) {
        std::unique_ptr<Io> file{Io::open(path, Io::eRead)};
        return file->read<std::byte>();
    }

    D3D12_SHADER_BYTECODE getShader(const ShaderBlob &blob) {
        return D3D12_SHADER_BYTECODE {blob.data(), blob.size()};
    }
    
    Display createDisplay(const system::Size& size) {
        auto [width, height] = size;

        Display display = {
            .viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, float(width), float(height)),
            .scissor = CD3DX12_RECT(0, 0, LONG(width), LONG(height))
        };

        return display;
    }

    Display createLetterBoxDisplay(const system::Size& render, const system::Size& present) {
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

        D3D12_VIEWPORT viewport = CD3DX12_VIEWPORT();

        D3D12_RECT scissor = CD3DX12_RECT();

        viewport.TopLeftX = presentWidth * (1.f - x) / 2.f;
        viewport.TopLeftY = presentHeight * (1.f - y) / 2.f;
        viewport.Width = x * presentWidth;
        viewport.Height = y * presentHeight;

        scissor.left = LONG(viewport.TopLeftX);
        scissor.right = LONG(viewport.TopLeftX + viewport.Width);
        scissor.top = LONG(viewport.TopLeftY);
        scissor.bottom = LONG(viewport.TopLeftY + viewport.Height);

        return { viewport, scissor };
    }
}

struct Input {
    Input() : gamepad(0), keyboard(), mouse(false, true) {
        manager.add(&gamepad);
        manager.add(&keyboard);
        manager.add(&mouse);
    }

    void poll() {
        manager.poll();
    }

    input::Gamepad gamepad;
    input::Keyboard keyboard;
    input::Mouse mouse;

    input::Manager manager;
};

struct Info {
    system::Size windowResolution;
    system::Size renderResolution;

    Locale locale;
};

struct Window final : system::Window {
    Window(input::Mouse& mouse, input::Keyboard& keyboard)
        : system::Window("game", { 1920, 1080 })
        , mouse(mouse)
        , keyboard(keyboard)
    { }

    bool poll() {
        mouse.update(getHandle());
        return system::Window::poll();
    }

    LRESULT onEvent(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) override {
        keyboard.update(msg, wparam, lparam);

        return ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam);
    }

    input::Mouse& mouse;
    input::Keyboard& keyboard;
};

struct RenderEdge final : render::OutEdge {
    RenderEdge(const GraphObject& self, render::Pass *pPass)
        : OutEdge(self, pPass)
    { }

    ID3D12Resource *getResource() override {
        return getContext().getRenderTargetResource();
    }

    D3D12_RESOURCE_STATES getState() const override {
        return D3D12_RESOURCE_STATE_PRESENT;
    }

    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle() override {
        return getContext().getCpuTargetHandle();
    }

    D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle() override {
        return getContext().getGpuTargetHandle();
    }
};

struct SourceEdge final : render::OutEdge {
    SourceEdge(const GraphObject& self, render::Pass *pPass, D3D12_RESOURCE_STATES state)
        : SourceEdge(self, pPass, state, nullptr, { }, { })
    { }

    SourceEdge(const GraphObject& self, render::Pass *pPass, D3D12_RESOURCE_STATES state, ID3D12Resource *pResource, D3D12_CPU_DESCRIPTOR_HANDLE cpu, D3D12_GPU_DESCRIPTOR_HANDLE gpu)
        : OutEdge(self, pPass)
        , state(state)
        , pResource(pResource)
        , cpu(cpu)
        , gpu(gpu)
    { }

    void setResource(ID3D12Resource *pOther, D3D12_CPU_DESCRIPTOR_HANDLE otherCpu, D3D12_GPU_DESCRIPTOR_HANDLE otherGpu) {
        pResource = pOther;
        cpu = otherCpu;
        gpu = otherGpu;
    }

    ID3D12Resource *getResource() override { return pResource; }
    D3D12_RESOURCE_STATES getState() const override { return state; }
    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle() override { return cpu; }
    D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle() override { return gpu; }

private:
    D3D12_RESOURCE_STATES state;
    ID3D12Resource *pResource;
    D3D12_CPU_DESCRIPTOR_HANDLE cpu;
    D3D12_GPU_DESCRIPTOR_HANDLE gpu;
};

struct GlobalPass final : render::Pass {
    GlobalPass(const GraphObject& object) : Pass(object) { 
        pRenderTargetOut = out<RenderEdge>("render-target");
    }

    void execute() override { }

    render::OutEdge *pRenderTargetOut = nullptr;
};

struct ScenePass final : render::Pass {
    ScenePass(const GraphObject& object, const system::Size& size) 
        : Pass(object)
        , size(size)
    {
        pRenderTargetOut = out<SourceEdge>("scene-target", D3D12_RESOURCE_STATE_RENDER_TARGET);
    }

    void start() override {
        auto& ctx = getContext();
        auto device = ctx.getDevice();
        auto& rtvHeap = ctx.getRtvHeap();
        auto& cbvHeap = ctx.getCbvHeap();

        rtvIndex = rtvHeap.alloc();
        cbvIndex = cbvHeap.alloc();

        ID3D12Resource *pResource = nullptr;

        D3D12_CLEAR_VALUE clear = CD3DX12_CLEAR_VALUE(DXGI_FORMAT_R8G8B8A8_UNORM, kClearColour);

        // create intermediate render target
        D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Tex2D(
            /* format = */ DXGI_FORMAT_R8G8B8A8_UNORM,
            /* width = */ UINT(size.width),
            /* height = */ UINT(size.height),
            /* arraySize = */ 1,
            /* mipLevels = */ 1,
            /* sampleCount = */ 1,
            /* sampleQuality = */ 0,
            /* flags = */ D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET
        );

        D3D12_HEAP_PROPERTIES props = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

        HR_CHECK(device->CreateCommittedResource(
            /* heapProperties = */ &props,
            /* heapFlags = */ D3D12_HEAP_FLAG_NONE,
            /* pDesc = */ &desc,
            /* initialState = */ D3D12_RESOURCE_STATE_RENDER_TARGET,
            /* pOptimizedClearValue = */ &clear,
            /* riidResource = */ IID_PPV_ARGS(&pResource)
        ));

        // create render target view for the intermediate target
        device->CreateRenderTargetView(pResource, nullptr, rtvHeap.cpuHandle(rtvIndex));

        // create shader resource view of the intermediate render target
        device->CreateShaderResourceView(pResource, nullptr, cbvHeap.cpuHandle(cbvIndex));

        pRenderTargetOut->setResource(pResource, cbvHeap.cpuHandle(cbvIndex), cbvHeap.gpuHandle(cbvIndex));
    }

    void stop() override {
        auto& ctx = getContext();
        
        ctx.getCbvHeap().release(cbvIndex);
        ctx.getRtvHeap().release(rtvIndex);

        ID3D12Resource *pResource = pRenderTargetOut->getResource();
        RELEASE(pResource);

        pRenderTargetOut->setResource(nullptr, { }, { });
    }

    void execute() override {
        auto& ctx = getContext();
        auto& rtvHeap = ctx.getRtvHeap();
        auto cmd = getContext().getCommandList();
        auto rtv = rtvHeap.cpuHandle(rtvIndex); // TODO: definetly not right
        Display display = createDisplay(size);

        cmd->RSSetViewports(1, &display.viewport);
        cmd->RSSetScissorRects(1, &display.scissor);

        cmd->OMSetRenderTargets(1, &rtv, false, nullptr);
        cmd->ClearRenderTargetView(rtv, kClearColour, 0, nullptr);
    }

    SourceEdge *pRenderTargetOut = nullptr;

private:
    system::Size size;
    
    constexpr static float kClearColour[] = { 0.0f, 0.2f, 0.4f, 1.0f };

    // TODO: this should be part of SourceEdge maybe?
    render::Heap::Index cbvIndex = render::Heap::Index::eInvalid;
    render::Heap::Index rtvIndex = render::Heap::Index::eInvalid;
};

// copy resource to back buffer
struct BlitPass final : render::Pass {
    BlitPass(const GraphObject& object, const Display& display) 
        : Pass(object) 
        , display(display)
    {
        // create wires
        pSceneTargetIn = in<render::InEdge>("scene-target", D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        pRenderTargetIn = in<render::InEdge>("render-target", D3D12_RESOURCE_STATE_RENDER_TARGET);

        pSceneTargetOut = out<render::RelayEdge>("scene-target", pSceneTargetIn);
        pRenderTargetOut = out<render::RelayEdge>("render-target", pRenderTargetIn);

        // load shader objects
        vs = loadShader("build\\game\\game.exe.p\\post.vs.cso");
        ps = loadShader("build\\game\\game.exe.p\\post.ps.cso");
    }

    void start() override {
        auto& ctx = getContext();
        auto device = ctx.getDevice();

        // s0 is the sampler
        CD3DX12_STATIC_SAMPLER_DESC samplers[1];
        samplers[0].Init(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR);

        CD3DX12_DESCRIPTOR_RANGE1 range(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

        // t0 is the intermediate render target
        CD3DX12_ROOT_PARAMETER1 rootParameters[1];
        rootParameters[0].InitAsDescriptorTable(1, &range);

        CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
        rootSignatureDesc.Init(_countof(rootParameters), (D3D12_ROOT_PARAMETER*)rootParameters, _countof(samplers), samplers, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

        ID3DBlob *pSignature = nullptr;
        ID3DBlob *pError = nullptr;

        HR_CHECK(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &pSignature, &pError));
        HR_CHECK(device->CreateRootSignature(0, pSignature->GetBufferPointer(), pSignature->GetBufferSize(), IID_PPV_ARGS(&pBlitSignature)));

        RELEASE(pSignature);
        RELEASE(pError);

        D3D12_INPUT_ELEMENT_DESC layout[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
        };

        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {
            .pRootSignature = pBlitSignature,
            .VS = getShader(vs),
            .PS = getShader(ps),
            .BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT),
            .SampleMask = UINT_MAX,
            .RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT),
            .DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(),
            .InputLayout = { layout, _countof(layout) },
            .PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
            .NumRenderTargets = 1,
            .RTVFormats = { DXGI_FORMAT_R8G8B8A8_UNORM },
            .SampleDesc = { 1, 0 },
        };

        HR_CHECK(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pBlitPipeline)));
    
        // upload fullscreen quad vertex and index data
        D3D12_RESOURCE_DESC vertexBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(kScreenQuadVertices));
        D3D12_RESOURCE_DESC indexBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(kScreenQuadIndices));

        D3D12_HEAP_PROPERTIES props = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

        HR_CHECK(device->CreateCommittedResource(
            &props,
            D3D12_HEAP_FLAG_NONE,
            &vertexBufferDesc,
            D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
            nullptr,
            IID_PPV_ARGS(&pVertexBuffer)
        ));

        HR_CHECK(device->CreateCommittedResource(
            &props,
            D3D12_HEAP_FLAG_NONE,
            &indexBufferDesc,
            D3D12_RESOURCE_STATE_INDEX_BUFFER,
            nullptr,
            IID_PPV_ARGS(&pIndexBuffer)
        ));

        void *pVertexData = nullptr;
        void *pIndexData = nullptr;

        HR_CHECK(pVertexBuffer->Map(0, nullptr, &pVertexData));
        memcpy(pVertexData, kScreenQuadVertices, sizeof(kScreenQuadVertices));
        
        HR_CHECK(pIndexBuffer->Map(0, nullptr, &pIndexData));
        memcpy(pIndexData, kScreenQuadIndices, sizeof(kScreenQuadIndices));

        pVertexBuffer->Unmap(0, nullptr);
        pIndexBuffer->Unmap(0, nullptr);

        vertexBufferView.BufferLocation = pVertexBuffer->GetGPUVirtualAddress();
        vertexBufferView.StrideInBytes = sizeof(Vertex);
        vertexBufferView.SizeInBytes = sizeof(kScreenQuadVertices);

        indexBufferView.BufferLocation = pIndexBuffer->GetGPUVirtualAddress();
        indexBufferView.Format = DXGI_FORMAT_R16_UINT;
        indexBufferView.SizeInBytes = sizeof(kScreenQuadIndices);
    }

    void stop() override {
        RELEASE(pBlitSignature);
        RELEASE(pBlitPipeline);

        RELEASE(pVertexBuffer);
        RELEASE(pIndexBuffer);
    }

    void execute() override {
        auto& ctx = getContext();
        auto cmd = ctx.getCommandList();
        auto rtv = pRenderTargetIn->cpuHandle();
        
        cmd->RSSetViewports(1, &display.viewport);
        cmd->RSSetScissorRects(1, &display.scissor);
        
        cmd->OMSetRenderTargets(1, &rtv, false, nullptr);
        cmd->ClearRenderTargetView(rtv, kClearColor, 0, nullptr);

        cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        cmd->IASetVertexBuffers(0, 1, &vertexBufferView);
        cmd->IASetIndexBuffer(&indexBufferView);

        cmd->SetPipelineState(pBlitPipeline);
        cmd->SetGraphicsRootSignature(pBlitSignature);

        cmd->SetGraphicsRootDescriptorTable(0, pSceneTargetIn->gpuHandle());
    
        cmd->DrawIndexedInstanced(_countof(kScreenQuadIndices), 1, 0, 0, 0);
    }

    render::InEdge *pSceneTargetIn = nullptr;
    render::InEdge *pRenderTargetIn = nullptr;

    render::OutEdge *pSceneTargetOut = nullptr;
    render::OutEdge *pRenderTargetOut = nullptr;

private:
    Display display;

    constexpr static float kClearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };

    ShaderBlob ps;
    ShaderBlob vs;

    ID3D12RootSignature *pBlitSignature = nullptr;
    ID3D12PipelineState *pBlitPipeline = nullptr;

    constexpr static Vertex kScreenQuadVertices[] = {
        { { -1.0f, 1.0f, 0.0f }, { 0.0f, 0.0f } },
        { { 1.0f, 1.0f, 0.0f }, { 1.0f, 0.0f } },
        { { 1.0f, -1.0f, 0.0f }, { 1.0f, 1.0f } },
        { { -1.0f, -1.0f, 0.0f }, { 0.0f, 1.0f } }
    };

    constexpr static uint16_t kScreenQuadIndices[] = {
        0, 1, 2,
        0, 2, 3
    };

    // fullscreen quad
    ID3D12Resource *pVertexBuffer = nullptr;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView;

    ID3D12Resource *pIndexBuffer = nullptr;
    D3D12_INDEX_BUFFER_VIEW indexBufferView;
};

struct PresentPass final : render::Pass {
    PresentPass(const GraphObject& object) : Pass(object) { 
        pSceneTargetIn = in<render::InEdge>("scene-target", D3D12_RESOURCE_STATE_RENDER_TARGET);
        pRenderTargetIn = in<render::InEdge>("render-target", D3D12_RESOURCE_STATE_PRESENT);
    }

    void execute() override {
        // empty?
    }

    render::InEdge *pSceneTargetIn = nullptr;
    render::InEdge *pRenderTargetIn = nullptr;
};

struct ImGuiPass final : render::Pass {
    ImGuiPass(const GraphObject& object, Info& info, input::Manager& manager): Pass(object), info(info), inputManager(manager) { 
        pRenderTargetIn = in<render::InEdge>("render-target", D3D12_RESOURCE_STATE_RENDER_TARGET);
        pRenderTargetOut = out<render::RelayEdge>("render-target", pRenderTargetIn);
    }
    
    void start() override {
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

    void stop() override {
        ImNodes::DestroyContext();

        ImGui_ImplDX12_Shutdown();
        getContext().getCbvHeap().release(fontHandle);
    }

    void execute() override {
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

    render::InEdge *pRenderTargetIn = nullptr;
    render::OutEdge *pRenderTargetOut = nullptr;

private:
    Info& info;
    input::Manager& inputManager;
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

    void drawInfo() {
        if (ImGui::Begin("Info")) {
            auto display = createLetterBoxDisplay(info.renderResolution, info.windowResolution);

            ImGui::Text("Viewport(%f, %f, %f, %f)", display.viewport.TopLeftX, display.viewport.TopLeftY, display.viewport.Width, display.viewport.Height);
            ImGui::Text("Scissor(%ld, %ld, %ld, %ld)", display.scissor.left, display.scissor.top, display.scissor.right, display.scissor.bottom);

            auto [displayWidth, displayHeight] = info.windowResolution;
            ImGui::Text("Display: %zu x %zu", displayWidth, displayHeight);

            auto [renderWidth, renderHeight] = info.renderResolution;
            ImGui::Text("Render: %zu x %zu", renderWidth, renderHeight);
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

    void drawGdkInfo() {
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

    void drawInputInfo() {
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

    struct Link {
        int src;
        int dst;
    };

    std::unordered_map<render::Pass*, int> passIndices;
    std::unordered_map<render::Edge*, int> edgeIndices;
    std::unordered_map<int, Link> links;
};

struct Scene final : render::Graph {
    Scene(render::Context& context, Info& info, input::Manager& input) : render::Graph(context) {
        Display present = createLetterBoxDisplay(info.renderResolution, info.windowResolution);

        pGlobalPass = addPass<GlobalPass>("global");
        
        pScenePass = addPass<ScenePass>("scene", info.renderResolution);
        pBlitPass = addPass<BlitPass>("blit", present);

        pImGuiPass = addPass<ImGuiPass>("imgui", info, input);
        pPresentPass = addPass<PresentPass>("present");

        connect(pGlobalPass->pRenderTargetOut, pBlitPass->pRenderTargetIn);
        connect(pScenePass->pRenderTargetOut, pBlitPass->pSceneTargetIn);

        connect(pBlitPass->pRenderTargetOut, pImGuiPass->pRenderTargetIn);

        connect(pBlitPass->pSceneTargetOut, pPresentPass->pSceneTargetIn);
        connect(pImGuiPass->pRenderTargetOut, pPresentPass->pRenderTargetIn);
    }

    void execute() {
        Graph::execute(pPresentPass);
    }

private:
    GlobalPass *pGlobalPass;

    ScenePass *pScenePass;
    BlitPass *pBlitPass;

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
    gLog.info("cwd: {}", std::filesystem::current_path().string());

    system::System system;
    ImGuiRuntime imgui;
    game::gdk::Runtime gdk;
    Input input;

    Window window { input.mouse, input.keyboard };

    Info detail = {
        .windowResolution = window.size(),
        .renderResolution = { 800, 600 }
    };

    render::Context::Info info = {
        .windowSize = { 600, 800 }
    };
    render::Context context { window, info };
    Scene scene { context, detail, input.manager };

    ImGui_ImplWin32_Init(window.getHandle());
    ImGui_ImplWin32_EnableDpiAwareness();

    scene.start();

    while (window.poll()) {
        input.poll();
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
