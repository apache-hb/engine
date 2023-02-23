#include "game/render.h"
#include "simcoe/core/units.h"

using namespace game;
using namespace simcoe;

using namespace simcoe::units;

ScenePass::ScenePass(const GraphObject& object, Info& info)
    : Pass(object)
    , info(info)
{
    pRenderTargetOut = out<IntermediateTargetEdge>("scene-target", info.renderResolution);

    vs = loadShader("build\\game\\libgame.a.p\\scene.vs.cso");
    ps = loadShader("build\\game\\libgame.a.p\\scene.ps.cso");
}

void ScenePass::start() {
    pRenderTargetOut->start();

    auto& ctx = getContext();
    auto *pDevice = ctx.getDevice();
    auto& cbvHeap = ctx.getCbvHeap();

    CD3DX12_STATIC_SAMPLER_DESC samplers[1];
    samplers[0].Init(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR);

    CD3DX12_DESCRIPTOR_RANGE1 textureRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, UINT_MAX, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE);

    CD3DX12_ROOT_PARAMETER1 rootParameters[3];
    // t0[] textures
    rootParameters[0].InitAsDescriptorTable(1, &textureRange);

    // b0 camera buffer
    rootParameters[1].InitAsConstantBufferView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE);

    // b1 material buffer
    rootParameters[2].InitAsConstants(1, 1, 0);

    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    rootSignatureDesc.Init(
        UINT(std::size(rootParameters)), (D3D12_ROOT_PARAMETER*)rootParameters, 
        UINT(std::size(samplers)), samplers, 
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
    );

    ID3DBlob *pSignature = nullptr;
    ID3DBlob *pError = nullptr;

    HR_CHECK(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &pSignature, &pError));
    HR_CHECK(pDevice->CreateRootSignature(0, pSignature->GetBufferPointer(), pSignature->GetBufferSize(), IID_PPV_ARGS(&pRootSignature)));

    RELEASE(pSignature);
    RELEASE(pError);

    D3D12_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {
        .pRootSignature = pRootSignature,
        .VS = getShader(vs),
        .PS = getShader(ps),
        .BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT),
        .SampleMask = UINT_MAX,
        .RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT),
        .DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(),
        .InputLayout = { layout, UINT(std::size(layout)) },
        .PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
        .NumRenderTargets = 1,
        .RTVFormats = { DXGI_FORMAT_R8G8B8A8_UNORM },
        .SampleDesc = { 1, 0 },
    };

    HR_CHECK(pDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pPipelineState)));

    D3D12_RESOURCE_DESC cameraBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(SceneBuffer));
    D3D12_HEAP_PROPERTIES props = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

    HR_CHECK(pDevice->CreateCommittedResource(
        &props,
        D3D12_HEAP_FLAG_NONE,
        &cameraBufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&pSceneBuffer)
    ));

    sceneHandle = cbvHeap.alloc();

    D3D12_CONSTANT_BUFFER_VIEW_DESC bufferDesc = {
        .BufferLocation = pSceneBuffer->GetGPUVirtualAddress(),
        .SizeInBytes = sizeof(SceneBuffer)
    };

    pDevice->CreateConstantBufferView(&bufferDesc, cbvHeap.cpuHandle(sceneHandle));

    CD3DX12_RANGE read(0, 0);
    HR_CHECK(pSceneBuffer->Map(0, &read, (void**)&pSceneData));
}

void ScenePass::stop() {
    auto& ctx = getContext();

    ctx.getCbvHeap().release(sceneHandle);
    pSceneData = nullptr;
    pSceneBuffer->Unmap(0, nullptr);
    RELEASE(pSceneBuffer);

    RELEASE(pPipelineState);
    RELEASE(pRootSignature);

    pRenderTargetOut->stop();
}

void ScenePass::execute() {
    pSceneData->mvp = info.pCamera->mvp(float4x4::identity(), info.renderResolution.aspectRatio<float>());

    auto& ctx = getContext();
    auto cmd = ctx.getDirectCommands();
    auto& cbvHeap = ctx.getCbvHeap();

    auto rtv = pRenderTargetOut->cpuHandle(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    Display display = createDisplay(info.renderResolution);

    cmd->RSSetViewports(1, &display.viewport);
    cmd->RSSetScissorRects(1, &display.scissor);

    cmd->OMSetRenderTargets(1, &rtv, false, nullptr);
    cmd->ClearRenderTargetView(rtv, kClearColour, 0, nullptr);

    cmd->SetGraphicsRootSignature(pRootSignature);
    cmd->SetPipelineState(pPipelineState);

    cmd->SetGraphicsRootDescriptorTable(0, cbvHeap.gpuHandle());
    cmd->SetGraphicsRootConstantBufferView(1, pSceneBuffer->GetGPUVirtualAddress());
#if 0
    cmd->SetGraphicsRoot32BitConstant(2, UINT32(textureHandle), 0);

    cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmd->IASetVertexBuffers(0, 1, &vertexBufferView);
    cmd->IASetIndexBuffer(&indexBufferView);

    cmd->DrawIndexedInstanced(std::size(kCubeIndices), 1, 0, 0, 0);
#endif
}

UploadHandle::UploadHandle(ScenePass *pSelf, const fs::path& path) 
    : name(path.filename().string())
    , pSelf(pSelf)
{
    auto& ctx = pSelf->getContext();

    copyCommands = ctx.newCommandBuffer(D3D12_COMMAND_LIST_TYPE_COPY);
    directCommands = ctx.newCommandBuffer(D3D12_COMMAND_LIST_TYPE_DIRECT);

    upload = assets::gltf(path, *this);
}

size_t UploadHandle::getDefaultTexture() {
    return SIZE_MAX;
}

size_t UploadHandle::addVertexBuffer(std::span<const assets::Vertex>) {
    return SIZE_MAX;
}

size_t UploadHandle::addIndexBuffer(std::span<const uint16_t>) {
    return SIZE_MAX;
}

size_t UploadHandle::addTexture(const assets::Texture& texture) {
    const auto& [data, size] = texture;

    auto& ctx = pSelf->getContext();
    auto pDevice = ctx.getDevice();
    auto& cbvHeap = ctx.getCbvHeap();
    auto& textures = pSelf->textures;

    auto direct = directCommands.pCommandList;
    auto copy = copyCommands.pCommandList;

    std::lock_guard guard(pSelf->mutex);

    ID3D12Resource *pStagingTexture = nullptr;
    ID3D12Resource *pTexture = nullptr;

    D3D12_RESOURCE_DESC textureDesc = CD3DX12_RESOURCE_DESC::Tex2D(
        /* format = */ DXGI_FORMAT_R8G8B8A8_UNORM,
        /* width = */ UINT(size.x),
        /* height = */ UINT(size.y)
    );

    D3D12_HEAP_PROPERTIES defaultProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    D3D12_HEAP_PROPERTIES uploadProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

    HR_CHECK(pDevice->CreateCommittedResource(
        &defaultProps,
        D3D12_HEAP_FLAG_NONE,
        &textureDesc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(&pTexture)
    ));

    UINT64 uploadSize = GetRequiredIntermediateSize(pTexture, 0, 1);
    D3D12_RESOURCE_DESC uploadDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadSize);

    HR_CHECK(pDevice->CreateCommittedResource(
        &uploadProps,
        D3D12_HEAP_FLAG_NONE,
        &uploadDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&pStagingTexture)
    ));

    D3D12_SUBRESOURCE_DATA subresourceData = {
        .pData = data,
        .RowPitch = LONG_PTR(size.x * 4),
        .SlicePitch = LONG_PTR(size.x * size.y * 4)
    };

    D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        pTexture, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
    );

    UpdateSubresources(copy, pTexture, pStagingTexture, 0, 0, 1, &subresourceData);

    direct->ResourceBarrier(1, &barrier);
    
    auto handle = cbvHeap.alloc();
    size_t result = textures.size();

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {
        .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
        .ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D,
        .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
        .Texture2D = { .MipLevels = 1 }
    };

    pDevice->CreateShaderResourceView(pTexture, &srvDesc, cbvHeap.cpuHandle(handle));

    ctx.submitCopyCommands(copyCommands);
    ctx.submitDirectCommands(directCommands);

    textures.push_back({ 
        .name = std::format("texture {}", result), 
        .size = size, 
        .pResource = pTexture, 
        .handle = handle 
    });

    gRenderLog.info("added texture {} ({}x{})", result, size.x, size.y);

    return result;
}

size_t UploadHandle::addMesh(const assets::Mesh&) {
    return SIZE_MAX;
}

size_t UploadHandle::addNode(const assets::Node&) {
    return SIZE_MAX;
}

void ScenePass::addUpload(const fs::path& path) {
    std::lock_guard guard(mutex);
    
    uploads.emplace_back(new UploadHandle(this, path));
}
