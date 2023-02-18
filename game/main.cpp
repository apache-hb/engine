#include <filesystem>

#include "simcoe/core/system.h"
#include "simcoe/core/win32.h"
#include "simcoe/core/io.h"

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

//#include "GameInput.h"
//#include "XGameRuntime.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

using namespace simcoe;

namespace {
    using ShaderBlob = std::vector<std::byte>;

    struct Vertex {
        math::float3 position;
        math::float2 uv;
    };

    ShaderBlob loadShader(std::string_view path) {
        std::unique_ptr<Io> file{Io::open(path, Io::eRead)};
        return file->read<std::byte>();
    }

    D3D12_SHADER_BYTECODE getShader(ShaderBlob &blob) {
        return D3D12_SHADER_BYTECODE {blob.data(), blob.size()};
    }
}

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
    Info() : gamepad(0), keyboard(), mouse(false, true) {
        manager.add(&gamepad);
        manager.add(&keyboard);
        manager.add(&mouse);
    }

    void poll() {
        manager.poll();
    }

    system::Size windowResolution;
    system::Size internalResolution;

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
    { 
        info.windowResolution = Window::size();
        info.internalResolution = { 1920, 1080 };
    }

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

struct Display {
    D3D12_VIEWPORT viewport;
    D3D12_RECT scissor;
};

Display createDisplay(const math::size2& size) {
    auto [width, height] = size;

    Display display;
    display.viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, float(width), float(height));
    display.scissor = CD3DX12_RECT(0, 0, LONG(width), LONG(height));

    return display;
}

Display createLetterBoxDisplay(const math::size2& internal, const math::size2& external) {
    auto [sceneWidth, sceneHeight] = internal;
    auto [windowWidth, windowHeight] = external;

    auto widthRatio = float(sceneWidth) / windowWidth;
    auto heightRatio = float(sceneHeight) / windowHeight;

    float x = 1.f;
    float y = 1.f;

    if (widthRatio < heightRatio) {
        x = widthRatio / heightRatio;
    } else {
        y = heightRatio / widthRatio;
    }

    D3D12_VIEWPORT view = CD3DX12_VIEWPORT(
        /* topLeftX = */ float(windowWidth) * (1.f - x) / 2.f,
        /* topLeftY = */ float(windowHeight) * (1.f - y) / 2.f,
        /* width = */ x * float(windowWidth),
        /* height = */ y * float(windowHeight)
    );

    D3D12_RECT scissor = CD3DX12_RECT(
        /* left = */ LONG(view.TopLeftX),
        /* top = */ LONG(view.TopLeftY),
        /* right = */ LONG(view.TopLeftX + view.Width),
        /* bottom = */ LONG(view.TopLeftY + view.Height)
    );

    return { view, scissor };
}

struct GlobalPass final : render::Pass {
    GlobalPass(const GraphObject& object) : Pass(object) { 
        pRenderTargetOut = out<RenderEdge>("render-target");
    }

    void execute() override { }

    render::OutEdge *pRenderTargetOut = nullptr;
};

struct ScenePass final : render::Pass {
    ScenePass(const GraphObject& object, const Display& display) 
        : Pass(object)
        , scene(display) 
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
            /* width = */ UINT(scene.viewport.Width),
            /* height = */ UINT(scene.viewport.Height),
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

        cmd->RSSetViewports(1, &scene.viewport);
        cmd->RSSetScissorRects(1, &scene.scissor);

        cmd->OMSetRenderTargets(1, &rtv, false, nullptr);
        cmd->ClearRenderTargetView(rtv, kClearColour, 0, nullptr);
    }

    SourceEdge *pRenderTargetOut = nullptr;

private:
    Display scene;
    
    constexpr static float kClearColour[] = { 0.0f, 0.2f, 0.4f, 1.0f };

    // TODO: this should be part of SourceEdge maybe?
    render::Heap::Index cbvIndex = render::Heap::Index::eInvalid;
    render::Heap::Index rtvIndex = render::Heap::Index::eInvalid;
};

// copy resource to back buffer
struct BlitPass final : render::Pass {
    BlitPass(const GraphObject& object,const math::size2& windowSize) 
        : Pass(object) 
        , window(createDisplay(windowSize))
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
        // TODO: create pipeline state

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
            .DSVFormat = DXGI_FORMAT_UNKNOWN,
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
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&pVertexBuffer)
        ));

        HR_CHECK(device->CreateCommittedResource(
            &props,
            D3D12_HEAP_FLAG_NONE,
            &indexBufferDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
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
        
        cmd->RSSetViewports(1, &window.viewport);
        cmd->RSSetScissorRects(1, &window.scissor);
        
        cmd->OMSetRenderTargets(1, &rtv, false, nullptr);

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
    Display window;

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

constexpr const char *stateToString(D3D12_RESOURCE_STATES states) {
    switch (states) {
    case D3D12_RESOURCE_STATE_COMMON: return "common/present";
    case D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER: return "vertex-and-constant-buffer";
    case D3D12_RESOURCE_STATE_INDEX_BUFFER: return "index-buffer";
    case D3D12_RESOURCE_STATE_RENDER_TARGET: return "render-target";
    case D3D12_RESOURCE_STATE_UNORDERED_ACCESS: return "unordered-access";
    case D3D12_RESOURCE_STATE_DEPTH_WRITE: return "depth-write";
    case D3D12_RESOURCE_STATE_DEPTH_READ: return "depth-read";
    case D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE: return "non-pixel-shader-resource";
    case D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE: return "pixel-shader-resource";
    case D3D12_RESOURCE_STATE_STREAM_OUT: return "stream-out";
    case D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT: return "indirect-argument";
    case D3D12_RESOURCE_STATE_COPY_DEST: return "copy-dest";
    case D3D12_RESOURCE_STATE_COPY_SOURCE: return "copy-source";
    case D3D12_RESOURCE_STATE_RESOLVE_DEST: return "resolve-dest";
    case D3D12_RESOURCE_STATE_RESOLVE_SOURCE: return "resolve-source";
    case D3D12_RESOURCE_STATE_GENERIC_READ: return "generic-read";
    default: return "unknown";
    }
}

struct ImGuiPass final : render::Pass {
    ImGuiPass(const GraphObject& object, Info& info): Pass(object), info(info) { 
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
                ImGui::MenuItem("Open");

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
        Display display = createLetterBoxDisplay(info.internalResolution, info.windowResolution);
        pGlobalPass = addPass<GlobalPass>("global");
        
        pScenePass = addPass<ScenePass>("scene", display);
        pBlitPass = addPass<BlitPass>("blit", info.windowResolution);

        pImGuiPass = addPass<ImGuiPass>("imgui", info);
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
    Info detail;

    Window window { detail };

    render::Context::Info info = {
        .windowSize = { 600, 800 }
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
