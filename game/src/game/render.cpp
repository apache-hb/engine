#include "game/render.h"
#include "game/registry.h"

#include "simcoe/core/io.h"
#include "simcoe/simcoe.h"

#include "imgui/imgui.h"
#include "imnodes/imnodes.h"

using namespace simcoe;
using namespace game;

void game::uploadData(ID3D12Resource *pResource, size_t size, const void *pData) {
    void *ptr = nullptr;
    HR_CHECK(pResource->Map(0, nullptr, &ptr));
    memcpy(ptr, pData, size);
    pResource->Unmap(0, nullptr);
}

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

D3D12_CPU_DESCRIPTOR_HANDLE RenderEdge::cpuHandle(D3D12_DESCRIPTOR_HEAP_TYPE type) {
    ASSERT(type == D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    
    auto& ctx = getContext();

    auto& data = frameData[ctx.getCurrentFrame()];
    return ctx.getRtvHeap().cpuHandle(data.index);
}

D3D12_GPU_DESCRIPTOR_HANDLE RenderEdge::gpuHandle(D3D12_DESCRIPTOR_HEAP_TYPE type) {
    ASSERT(type == D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    auto& ctx = getContext();
    
    auto& data = frameData[ctx.getCurrentFrame()];
    return ctx.getRtvHeap().gpuHandle(data.index);
}

void RenderEdge::start() {
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

void RenderEdge::stop() {
    auto& rtvHeap = getContext().getRtvHeap();

    for (auto &frame : frameData) {
        RELEASE(frame.pRenderTarget);
        rtvHeap.release(frame.index);
    }
}

IntermediateTargetEdge::IntermediateTargetEdge(const GraphObject& self, render::Pass *pPass, const system::Size& size)
    : OutEdge(self, pPass)
    , size(size)
{ }

void IntermediateTargetEdge::start() {
    auto& ctx = getContext();
    auto *pDevice = ctx.getDevice();

    rtvIndex = ctx.getRtvHeap().alloc();
    cbvIndex = ctx.getCbvHeap().alloc();

    state = D3D12_RESOURCE_STATE_RENDER_TARGET;

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

    HR_CHECK(pDevice->CreateCommittedResource(
        /* heapProperties = */ &props,
        /* heapFlags = */ D3D12_HEAP_FLAG_CREATE_NOT_ZEROED,
        /* pDesc = */ &desc,
        /* initialState = */ D3D12_RESOURCE_STATE_RENDER_TARGET,
        /* pOptimizedClearValue = */ &clear,
        /* riidResource = */ IID_PPV_ARGS(&pResource)
    ));

    // create render target view for the intermediate target
    pDevice->CreateRenderTargetView(pResource, nullptr, cpuHandle(D3D12_DESCRIPTOR_HEAP_TYPE_RTV));

    // create shader resource view of the intermediate render target
    pDevice->CreateShaderResourceView(pResource, nullptr, cpuHandle(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
}

void IntermediateTargetEdge::stop() {
    auto& ctx = getContext();

    RELEASE(pResource);
    ctx.getRtvHeap().release(rtvIndex);
    ctx.getCbvHeap().release(cbvIndex);
}

ID3D12Resource *IntermediateTargetEdge::getResource() { 
    return pResource; 
}

D3D12_RESOURCE_STATES IntermediateTargetEdge::getState() const { 
    return state; 
}

D3D12_CPU_DESCRIPTOR_HANDLE IntermediateTargetEdge::cpuHandle(D3D12_DESCRIPTOR_HEAP_TYPE type) { 
    auto& ctx = getContext();

    switch (type) {
    case D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV:
        return ctx.getCbvHeap().cpuHandle(cbvIndex);
    case D3D12_DESCRIPTOR_HEAP_TYPE_RTV:
        return ctx.getRtvHeap().cpuHandle(rtvIndex);

    default:
        ASSERT(false);
    }
}

D3D12_GPU_DESCRIPTOR_HANDLE IntermediateTargetEdge::gpuHandle(D3D12_DESCRIPTOR_HEAP_TYPE type) { 
    auto& ctx = getContext();
    switch (type) {
    case D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV:
        return ctx.getCbvHeap().gpuHandle(cbvIndex);
    case D3D12_DESCRIPTOR_HEAP_TYPE_RTV:
        return ctx.getRtvHeap().gpuHandle(rtvIndex);

    default:
        ASSERT(false);
    }
}

Scene::Scene(render::Context& context, Info& info, input::Manager& input) : render::Graph(context) {
    Display present = createLetterBoxDisplay(info.renderResolution, info.windowResolution);

    pGlobalPass = addPass<GlobalPass>("global");
    
    pScenePass = addPass<ScenePass>("scene", info);
    pBlitPass = addPass<BlitPass>("blit", present);

    pImGuiPass = addPass<ImGuiPass>("imgui", info, input);
    pPresentPass = addPass<PresentPass>("present");

    connect(pGlobalPass->pRenderTargetOut, pBlitPass->pRenderTargetIn);
    connect(pScenePass->pRenderTargetOut, pBlitPass->pSceneTargetIn);

    connect(pBlitPass->pRenderTargetOut, pImGuiPass->pRenderTargetIn);

    connect(pBlitPass->pSceneTargetOut, pPresentPass->pSceneTargetIn);
    connect(pImGuiPass->pRenderTargetOut, pPresentPass->pRenderTargetIn);

    ImNodes::CreateContext();
    ImNodes::LoadCurrentEditorStateFromIniFile("imnodes.ini");

    struct Link {
        int src;
        int dst;
    };

    std::unordered_map<render::Pass*, int> passIndices;
    std::unordered_map<render::Edge*, int> edgeIndices;
    std::unordered_map<int, Link> links;

    int id = 0;
    for (auto& [name, pass] : getPasses()) {
        passIndices[pass.get()] = id++;
        for (auto& in : pass->getInputs()) {
            edgeIndices[in.get()] = id++;
        }

        for (auto& out : pass->getOutputs()) {
            edgeIndices[out.get()] = id++;
        }
    }

    int link = 0;
    for (auto& [src, dst] : getEdges()) {
        int srcId = edgeIndices[src];
        int dstId = edgeIndices[dst];

        links[link++] = { srcId, dstId };
    }

    debug = game::debug.newEntry({ "Render Graph" }, [=, this] {
        ImNodes::BeginNodeEditor();

        for (auto& [name, pass] : getPasses()) {
            ImNodes::BeginNode(passIndices.at(pass.get()));

            ImNodes::BeginNodeTitleBar();
            ImGui::Text("%s", name);
            ImNodes::EndNodeTitleBar();

            for (auto& output : pass->getOutputs()) {
                ImNodes::BeginOutputAttribute(edgeIndices.at(output.get()));
                ImGui::Text("%s", output->getName());
                ImNodes::EndOutputAttribute();
            }

            for (auto& input : pass->getInputs()) {
                ImNodes::BeginInputAttribute(edgeIndices.at(input.get()));
                ImGui::Text("%s", input->getName());
                ImNodes::EndInputAttribute();
            }

            ImNodes::EndNode();
        }

        for (auto& [id, link] : links) {
            ImNodes::Link(id, link.src, link.dst);
        }

        ImNodes::EndNodeEditor();
    });
}

void Scene::load(const std::filesystem::path &path) {
    pScenePass->addUpload(path);
}
