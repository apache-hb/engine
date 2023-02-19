#include "game/render.h"
#include "simcoe/core/io.h"

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