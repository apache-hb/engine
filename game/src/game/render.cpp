#include "game/render.h"

#include "fastgltf/fastgltf_util.hpp"
#include "simcoe/core/io.h"
#include "simcoe/simcoe.h"

#include <fastgltf/fastgltf_parser.hpp>

using namespace simcoe;
using namespace game;

namespace {
    constexpr fastgltf::Options kOptions = fastgltf::Options::LoadGLBBuffers | fastgltf::Options::LoadExternalBuffers;

    constexpr const char *gltfErrorToString(fastgltf::Error err) {
#define ERROR_CASE(x) case fastgltf::Error::x: return #x
        switch (err) {
        ERROR_CASE(None);
        ERROR_CASE(InvalidPath);
        ERROR_CASE(MissingExtensions);
        ERROR_CASE(UnknownRequiredExtension);
        ERROR_CASE(InvalidJson);
        ERROR_CASE(InvalidGltf);
        case fastgltf::Error::InvalidOrMissingAssetField: 
            return "InvalidOrMissingAssetField/InvalidGLB";
        ERROR_CASE(MissingField);
        ERROR_CASE(MissingExternalBuffer);
        ERROR_CASE(UnsupportedVersion);
        default: return "Unknown";
        }
    }
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

void Scene::load(const std::filesystem::path& path) {
    fastgltf::Parser parser;
    fastgltf::GltfDataBuffer buffer;
    std::unique_ptr<fastgltf::glTF> data;

    buffer.loadFromFile(path);

    switch (fastgltf::determineGltfFileType(&buffer)) {
    case fastgltf::GltfType::glTF:
        gRenderLog.info("Loading glTF file: {}", path.string());
        data = parser.loadGLTF(&buffer, path.parent_path(), kOptions);
        break;
    case fastgltf::GltfType::GLB:
        gRenderLog.info("Loading GLB file: {}", path.string());
        data = parser.loadBinaryGLTF(&buffer, path.parent_path(), kOptions);
        break;

    default:
        gRenderLog.warn("Unknown glTF file type");
        return;
    }

    if (fastgltf::Error err = parser.getError(); err != fastgltf::Error::None) {
        gRenderLog.warn("Failed to load glTF file: {} ({})", gltfErrorToString(err), fastgltf::to_underlying(err));
        return;
    }

    if (fastgltf::Error err = data->parse(); err != fastgltf::Error::None) {
        gRenderLog.warn("Failed to parse glTF file: {} ({})", gltfErrorToString(err), fastgltf::to_underlying(err));
        return;
    }

    pScenePass->load(data->getParsedAsset());
}
