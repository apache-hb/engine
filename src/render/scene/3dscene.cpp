#include "scene.h"

#include <array>
#include <span>

#include "logging/debug.h"
#include "render/render.h"
#include "render/objects/commands.h"
#include "assets/texture.h"
#include "assets/mesh.h"
#include "imgui/imgui.h"

using namespace engine;
using namespace engine::math;
using namespace engine::render;

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
    root32BitParameter(visibility::kPixel, 1, 2), // cbuffer register(b1)
    tableParameter(visibility::kPixel, kSrvRanges) // Texture2D[] register(t0)
});

D3D12_TEXTURE_ADDRESS_MODE getSamplerMode(engine::assets::Sampler::Wrap wrap) {
    switch (wrap) {
    case engine::assets::Sampler::Wrap::REPEAT:
        return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    case engine::assets::Sampler::Wrap::CLAMP:
        return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    case engine::assets::Sampler::Wrap::MIRROR:
        return D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
    default:
        return D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    }
}

std::vector<D3D12_STATIC_SAMPLER_DESC> buildSamplers(std::span<const engine::assets::Sampler> data) {
    std::vector<D3D12_STATIC_SAMPLER_DESC> samplers(data.size());

    D3D12_STATIC_SAMPLER_DESC sampler = {
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
    };

    for (size_t i = 0; i < data.size(); i++) {
        const auto& it = data[i];
        sampler.AddressU = getSamplerMode(it.wrapU);
        sampler.AddressV = getSamplerMode(it.wrapV);
        sampler.ShaderRegister = UINT(i);
        samplers[i] = sampler;
    }

    return samplers;
}

constexpr D3D12_ROOT_SIGNATURE_FLAGS kFlags =
    D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
    D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS       |
    D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS     |
    D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

Scene3D::Scene3D(Context* context, Camera* camera, const assets::World* world): Scene(context), camera(camera), world(world) {
    shaders = ShaderLibrary(kShaderInfo);
}

void Scene3D::create() {
    createDsvHeap();
    createCbvSrvHeap();
    createConstBuffers();
    createDepthStencil();
    createRootSignature();
    createPipelineState();
    createCommandList();
    createSceneData();
}

void Scene3D::destroy() {
    destroySceneData();
    destroyCommandList();
    destroyPipelineState();
    destroyRootSignature();
    destroyDepthStencil();
    destroyConstBuffers();
    destroyCbvSrvHeap();
    destroyDsvHeap();
}

ID3D12CommandList* Scene3D::populate() {
    begin();
    
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    
    size_t numVertexBuffers = std::size(vertexBuffers);
    size_t numMeshes = std::size(world->meshes);
    float aspect = ctx->getInternalResolution().aspectRatio();

    for (size_t i = 0; i < numVertexBuffers; i++) {
        const auto& buffer = vertexBuffers[i];
        commandList->IASetVertexBuffers(UINT(i), 1, &buffer.view);
    }

    for (size_t i = 0; i < numMeshes; i++) {
        const auto& mesh = world->meshes[i];
        const auto& indexBuffer = indexBuffers[mesh.buffer];
        auto& transform = transforms[i];

        transform.data->mvp = camera->getMvp(transform.model, aspect);

        UINT values[] = { UINT(mesh.material.texture), UINT(mesh.material.sampler) };

        commandList->SetGraphicsRootConstantBufferView(0, transform.resource.gpuAddress());
        commandList->SetGraphicsRoot32BitConstants(1, 2, values, 0);
        commandList->IASetIndexBuffer(&indexBuffer.view);

        for (const auto& view : mesh.views) {
            commandList->DrawIndexedInstanced(UINT(view.length), 1, UINT(view.offset), 0, 0);
        }
    }

    end();
    return commandList.get();
}

void Scene3D::begin() {
    /*
    cameraData->mvp = camera->getMvp(float4x4::scaling(1.f, 1.f, 1.f), ctx->getInternalResolution().aspectRatio());
    camera->store(&cameraBuffer.view, &cameraBuffer.projection, ctx->getInternalResolution().aspectRatio());
    *cameraData = cameraBuffer;
    */

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

    commandList->SetGraphicsRootDescriptorTable(2, cbvSrvHeap.gpuHandle(SceneData::Total));
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

void Scene3D::createConstBuffers() {
    auto& device = getDevice();
    UINT size = sizeof(CameraBuffer);
    const auto buffer = CD3DX12_RESOURCE_DESC::Buffer(size);

    size_t numMeshes = std::size(world->meshes);
    transforms.resize(numMeshes);

    for (size_t i = 0; i < numMeshes; i++) {
        const auto name = std::format(L"transform-{}", i);
        auto resource = device.newCommittedResource(name,
            kUploadProps, D3D12_HEAP_FLAG_NONE,
            buffer, D3D12_RESOURCE_STATE_GENERIC_READ
        );

        D3D12_CONSTANT_BUFFER_VIEW_DESC view = {
            .BufferLocation = resource.gpuAddress(),
            .SizeInBytes = size
        };

        device->CreateConstantBufferView(&view, nodeCpuHandle(UINT(i)));
        auto* data = resource.map<CameraBuffer>(0);

        transforms[i] = { world->meshes[i].transform, data, resource };
    }
}

void Scene3D::destroyConstBuffers() {
    for (auto& transform : transforms) {
        transform.resource.tryDrop("transform");
    }
    transforms.clear();
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
    auto samplers = buildSamplers(world->samplers);
    RootCreate rootInfo = {
        .params = kParams,
        .samplers = samplers,
        .flags = kFlags
    };
    rootSignature = getDevice().newRootSignature(L"scene-root-signature", rootInfo);
}

void Scene3D::destroyRootSignature() {
    rootSignature.tryDrop("scene-root-signature");
}

void Scene3D::createPipelineState() {
    pipelineState = getDevice().newGraphicsPSO(L"viewport-pipeline-state", rootSignature, {
        .shaders = shaders,
        .dsvDesc = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT),
        .dsvFormat = kDepthFormat
    });
}

void Scene3D::destroyPipelineState() {
    pipelineState.tryDrop("viewport-pipeline-state");
}

void Scene3D::createSceneData() {
    size_t numVertexBuffers = std::size(world->vertexBuffers);
    size_t numIndexBuffers = std::size(world->indexBuffers);
    size_t numTextures = std::size(world->textures);

    vertexBuffers.resize(numVertexBuffers);
    indexBuffers.resize(numIndexBuffers);

    textures.resize(numTextures);

    for (size_t i = 0; i < numVertexBuffers; i++) {
        const auto& vertices = world->vertexBuffers[i];
        vertexBuffers[i] = ctx->uploadVertexBuffer<assets::Vertex>(std::format(L"vertex-buffer-{}", i), vertices);
    }

    for (size_t i = 0; i < numIndexBuffers; i++) {
        const auto& indices = world->indexBuffers[i];
        indexBuffers[i] = ctx->uploadIndexBuffer(std::format(L"index-buffer-{}", i), indices);
    }

    for (size_t i = 0; i < numTextures; i++) {
        const auto& texture = world->textures[i];
        auto tex = ctx->uploadTexture(texture);
        ctx->bindSrv(tex, cbvSrvHeap.cpuHandle(UINT(SceneData::Total + i)));

        textures[i] = tex;
    }
}

void Scene3D::destroySceneData() {
    for (auto& buffer : vertexBuffers) {
        buffer.tryDrop("vertex-buffer");
    }

    for (auto& buffer : indexBuffers) {
        buffer.tryDrop("index-buffer");
    }

    for (auto& texture : textures) {
        texture.tryDrop("texture");
    }
}

UINT Scene3D::getRequiredCbvSrvSize() const {
    return UINT(SceneData::Total + std::size(world->textures) + std::size(world->meshes));
}

CD3DX12_CPU_DESCRIPTOR_HANDLE Scene3D::cbvSrvCpuHandle(UINT index) {
    return cbvSrvHeap.cpuHandle(index + SceneData::Total);
}

CD3DX12_GPU_DESCRIPTOR_HANDLE Scene3D::cbvSrvGpuHandle(UINT index) {
    return cbvSrvHeap.gpuHandle(index + SceneData::Total);
}

CD3DX12_CPU_DESCRIPTOR_HANDLE Scene3D::nodeCpuHandle(UINT i) {
    return cbvSrvCpuHandle(UINT(std::size(world->textures) + i));
}

CD3DX12_CPU_DESCRIPTOR_HANDLE Scene3D::dsvHandle() {
    return CD3DX12_CPU_DESCRIPTOR_HANDLE(dsvHeap->GetCPUDescriptorHandleForHeapStart());
}

Device<ID3D12Device4>& Scene3D::getDevice() {
    return ctx->getDevice();
}
