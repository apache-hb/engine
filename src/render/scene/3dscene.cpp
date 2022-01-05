#include "scene.h"

#include <array>

#include "render/render.h"
#include "render/objects/commands.h"
#include "assets/texture.h"

using namespace engine;
using namespace engine::render;

const auto kMissingTexture = assets::genMissingTexture({ 512, 512 }, assets::Texture::Format::RGB);

constexpr auto kDepthFormat = DXGI_FORMAT_D32_FLOAT;
constexpr auto kUploadProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
constexpr auto kDefaultProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

constexpr D3D12_DEPTH_STENCIL_VIEW_DESC kDsvDesc = {
    .Format = kDepthFormat,
    .ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D
};
    
constexpr D3D12_CLEAR_VALUE kClearValue = {
    .Format = kDepthFormat,
    .DepthStencil = { .Depth = 1.0f, .Stencil = 0 }
};

constexpr auto kShaderLayout = std::to_array({
    shaderInput("POSITION", DXGI_FORMAT_R32G32B32_FLOAT),
    shaderInput("TEXCOORD", DXGI_FORMAT_R32G32_FLOAT)
});

constexpr ShaderLibrary::CreateBinary kShaderInfo = {
    .vsPath = L"resources\\shaders\\scene.vs.pso",
    .psPath = L"resources\\shaders\\scene.ps.pso",
    .layout = { kShaderLayout }
};

constexpr auto kSrvRanges = std::to_array({
    srvRange(0, UINT_MAX, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE)
});

constexpr auto kParams = std::to_array({
    cbvParameter(visibility::kVertex, 0), // cbuffer register(b0)
    root32BitParameter(visibility::kPixel, 1, 1), // cbuffer register(b1)
    tableParameter(visibility::kPixel, kSrvRanges) // Texture2D[] register(t0)
});

constexpr D3D12_STATIC_SAMPLER_DESC kSamplers[] = {{
    .Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR,
    .AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER,
    .AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER,
    .AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER,
    .ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER,
    .BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK,
    .MinLOD = 0.0f,
    .MaxLOD = D3D12_FLOAT32_MAX,
    .ShaderRegister = 0,
    .RegisterSpace = 0,
    .ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL
}};

constexpr D3D12_ROOT_SIGNATURE_FLAGS kFlags =
    D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
    D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS       |
    D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS     |
    D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

constexpr RootCreate kRootInfo = {
    .params = kParams,
    .samplers = kSamplers,
    .flags = kFlags
};

Scene3D::Scene3D(Context* context, Camera* camera): Scene(context), camera(camera) {
    shaders = ShaderLibrary(kShaderInfo);
    XMStoreFloat4x4(&cameraBuffer.model, XMMatrixScaling(1.f, 1.f, 1.f));
}

void Scene3D::create() {
    createDsvHeap();
    createCbvSrvHeap();
    createCameraBuffer();
    createDepthStencil();
    createRootSignature();
    createPipelineState();
    createCommandList();
}

void Scene3D::destroy() {
    destroyCommandList();
    destroyPipelineState();
    destroyRootSignature();
    destroyDepthStencil();
    destroyCameraBuffer();
    destroyCbvSrvHeap();
    destroyDsvHeap();
}

ID3D12CommandList* Scene3D::populate() {
    return commandList.get();
}

void Scene3D::begin() {
    camera->store(&cameraBuffer.view, &cameraBuffer.projection, ctx->getInternalResolution().aspectRatio());
    *cameraData = cameraBuffer;

    auto& allocator = ctx->getAllocator(Allocator::Scene);
    const auto [scissor, viewport] = ctx->getSceneView();
    auto targetHandle = ctx->sceneTargetRtvCpuHandle();
    auto depthHandle = dsvHandle();

    ID3D12DescriptorHeap* heaps[] = { cbvSrvHeap.get() };

    check(allocator->Reset(), "failed to reset allocator");
    check(commandList->Reset(allocator.get(), pipelineState.get()), "failed to reset command list");

    commandList->RSSetScissorRects(1, &scissor);
    commandList->RSSetViewports(1, &viewport);

    commandList->OMSetRenderTargets(1, &targetHandle, false, &depthHandle);
    commandList->ClearRenderTargetView(targetHandle, kClearColour, 0, nullptr);
    commandList->ClearDepthStencilView(depthHandle, D3D12_CLEAR_FLAG_DEPTH, 1.f, 0, 0, nullptr);

    commandList->SetDescriptorHeaps(UINT(std::size(heaps)), heaps);
    commandList->SetGraphicsRootSignature(rootSignature.get());
    
    // set root parameters
    //  - b0: camera buffer
    //  - b1: texture index (set in subclasses when drawing)
    //  - t0: texture array
    commandList->SetGraphicsRootConstantBufferView(0, cameraResource.gpuAddress());
    commandList->SetGraphicsRootDescriptorTable(2, cbvSrvGpuHandle(0));
}

void Scene3D::end() {
    check(commandList->Close(), "failed to close command list");
}

void Scene3D::createCommandList() {
    auto& allocator = ctx->getAllocator(Allocator::Scene);
    commandList = getDevice().newCommandList(L"scene-command-list", commands::kDirect, allocator);
    check(commandList->Close(), "failed to close command list");
}

void Scene3D::destroyCommandList() {
    commandList.tryDrop("scene-command-list");
}

void Scene3D::createDsvHeap() {
    dsvHeap = getDevice().newDescriptorHeap(L"scene-dsv-heap", {
        .Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
        .NumDescriptors = 1
    });
}

void Scene3D::createCbvSrvHeap() {
    cbvSrvHeap = getDevice().newDescriptorHeap(L"scene-cbv-srv-heap", {
        .Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
        .NumDescriptors = getRequiredCbvSrvSize(),
        .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
    });
}

void Scene3D::destroyCbvSrvHeap() {
    cbvSrvHeap.tryDrop("scene-cbv-srv-heap");
}

void Scene3D::destroyDsvHeap() {
    dsvHeap.tryDrop("scene-dsv-heap");
}

void Scene3D::createCameraBuffer() {
    auto& device = getDevice();
    UINT size = sizeof(CameraBuffer);
    const auto buffer = CD3DX12_RESOURCE_DESC::Buffer(size);

    cameraResource = device.newCommittedResource(L"camera-buffer",
        kUploadProps, D3D12_HEAP_FLAG_NONE,
        buffer, D3D12_RESOURCE_STATE_GENERIC_READ
    );

    D3D12_CONSTANT_BUFFER_VIEW_DESC view = {
        .BufferLocation = cameraResource.gpuAddress(),
        .SizeInBytes = size
    };

    device->CreateConstantBufferView(&view, cbvSrvHeap.cpuHandle(SceneData::Camera));

    cameraData = cameraResource.map<CameraBuffer>(0);
}

void Scene3D::destroyCameraBuffer() {
    cameraResource.tryDrop("camera-buffer");
}

void Scene3D::createDepthStencil() {
    auto& device = getDevice();
    const auto [width, height] = ctx->getInternalResolution();

    const auto tex = CD3DX12_RESOURCE_DESC::Tex2D(
        /* format = */ kDepthFormat,
        /* width = */ UINT(width),
        /* height = */ UINT(height),
        /* mipLevels = */ 1,
        /* arraySize = */ 0,
        /* sampleCount = */ 1,
        /* sampleQuality = */ 0,
        /* flags = */ D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL
    );

    depthStencil = device.newCommittedResource(L"scene-depth-stencil",
        kDefaultProps, D3D12_HEAP_FLAG_NONE,
        tex, D3D12_RESOURCE_STATE_DEPTH_WRITE, &kClearValue
    );

    device->CreateDepthStencilView(depthStencil.get(), &kDsvDesc, dsvHandle());
}

void Scene3D::destroyDepthStencil() {
    depthStencil.tryDrop("scene-depth-stencil");
}

void Scene3D::createRootSignature() {
    rootSignature = getDevice().newRootSignature(L"scene-root-signature", kRootInfo);
}

void Scene3D::destroyRootSignature() {
    rootSignature.tryDrop("scene-root-signature");
}

void Scene3D::createPipelineState() {
    pipelineState = getDevice().newGraphicsPSO(L"viewport-pipeline-state", rootSignature, {
        .shaders = shaders,
        .dsvFormat = kDepthFormat
    });
}

void Scene3D::destroyPipelineState() {
    pipelineState.tryDrop("viewport-pipeline-state");
}

UINT Scene3D::getRequiredCbvSrvSize() const {
    return SceneData::Total;
}

void Scene3D::imgui() {
    camera->imgui();
}

CD3DX12_CPU_DESCRIPTOR_HANDLE Scene3D::cbvSrvCpuHandle(UINT index) {
    return cbvSrvHeap.cpuHandle(index + SceneData::Total);
}

CD3DX12_GPU_DESCRIPTOR_HANDLE Scene3D::cbvSrvGpuHandle(UINT index) {
    return cbvSrvHeap.gpuHandle(index + SceneData::Total);
}

CD3DX12_CPU_DESCRIPTOR_HANDLE Scene3D::dsvHandle() {
    return CD3DX12_CPU_DESCRIPTOR_HANDLE(dsvHeap->GetCPUDescriptorHandleForHeapStart());
}

Device<ID3D12Device4>& Scene3D::getDevice() {
    return ctx->getDevice();
}
