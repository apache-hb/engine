#include "dx/d3d12.h"
#include "dx/d3dx12/d3dx12_core.h"
#include "game/render.h"
#include "game/registry.h"

#include "simcoe/assets/assets.h"
#include "simcoe/core/units.h"

using namespace game;
using namespace simcoe;

using namespace simcoe::units;

namespace {
    const D3D12_HEAP_PROPERTIES kUploadProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    const D3D12_HEAP_PROPERTIES kDefaultProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
}

ScenePass::ScenePass(const GraphObject& object, Info& info)
    : Pass(object)
    , info(info)
{
    pRenderTargetOut = out<IntermediateTargetEdge>("scene-target", info.renderResolution);

    vs = loadShader("build\\game\\libgame.a.p\\scene.vs.cso");
    ps = loadShader("build\\game\\libgame.a.p\\scene.ps.cso");

    debug = game::debug.newEntry({ "Scene" }, [this] {
        auto& ctx = getContext();
        auto& cbvHeap = ctx.getCbvHeap();

        ImGui::Text("Nodes: %zu", nodes.size());
        int root = int(rootNode);
        ImGui::InputInt("Root node", &root);
        rootNode = size_t(root);
        if (ImGui::BeginTable("textures", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY)) {
            ImGui::TableNextRow();
            for (const auto& [name, size, resource, handle] : textures) {
                ImGui::TableNextColumn();
                auto windowAvail = ImGui::GetContentRegionAvail();
                math::Resolution<float> res = { float(size.x), float(size.y) };

                ImGui::Text("%s: %zu x %zu", name.c_str(), size.x, size.y);
                ImGui::Image(ImTextureID(cbvHeap.gpuHandle(handle).ptr), ImVec2(windowAvail.x, res.aspectRatio<float>() * windowAvail.x));
            }
            ImGui::EndTable();
        }
    });
}

void ScenePass::start() {
    pRenderTargetOut->start();

    auto& ctx = getContext();
    auto *pDevice = ctx.getDevice();
    auto& cbvHeap = ctx.getCbvHeap();
    auto& dsvHeap = ctx.getDsvHeap();

    CD3DX12_STATIC_SAMPLER_DESC samplers[1];
    samplers[0].Init(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR);

    CD3DX12_DESCRIPTOR_RANGE1 textureRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, UINT_MAX, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE);

    CD3DX12_ROOT_PARAMETER1 rootParameters[4];
    // t0[] textures
    rootParameters[0].InitAsDescriptorTable(1, &textureRange);

    // b0 scene buffer
    rootParameters[1].InitAsConstantBufferView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE);

    // b1 material buffer
    rootParameters[2].InitAsConstants(1, 1, 0);

    // b2 node buffer
    rootParameters[3].InitAsConstantBufferView(2, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE);

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
        .DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT),
        .InputLayout = { layout, UINT(std::size(layout)) },
        .PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
        .NumRenderTargets = 1,
        .RTVFormats = { DXGI_FORMAT_R8G8B8A8_UNORM },
        .DSVFormat = DXGI_FORMAT_D32_FLOAT,
        .SampleDesc = { 1, 0 },
    };

    HR_CHECK(pDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pPipelineState)));

    D3D12_RESOURCE_DESC cameraBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(SceneBuffer));

    HR_CHECK(pDevice->CreateCommittedResource(
        &kUploadProps,
        D3D12_HEAP_FLAG_NONE,
        &cameraBufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&pSceneBuffer)
    ));

    // create depth stencil
    D3D12_RESOURCE_DESC depthStencilDesc = CD3DX12_RESOURCE_DESC::Tex2D(
        DXGI_FORMAT_D32_FLOAT,
        UINT(info.renderResolution.width),
        UINT(info.renderResolution.height),
        1, 0, 1, 0,
        D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL
    );
    
    D3D12_CLEAR_VALUE clearValue = {
        .Format = DXGI_FORMAT_D32_FLOAT,
        .DepthStencil = { 1.0f, 0 }
    };

    HR_CHECK(pDevice->CreateCommittedResource(
        &kDefaultProps,
        D3D12_HEAP_FLAG_NONE,
        &depthStencilDesc,
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &clearValue,
        IID_PPV_ARGS(&pDepthStencil)
    ));

    sceneHandle = cbvHeap.alloc();
    depthHandle = dsvHeap.alloc();

    D3D12_CONSTANT_BUFFER_VIEW_DESC bufferDesc = {
        .BufferLocation = pSceneBuffer->GetGPUVirtualAddress(),
        .SizeInBytes = sizeof(SceneBuffer)
    };

    D3D12_DEPTH_STENCIL_VIEW_DESC stencilDesc = {
        .Format = DXGI_FORMAT_D32_FLOAT,
        .ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D,
        .Flags = D3D12_DSV_FLAG_NONE
    };

    pDevice->CreateConstantBufferView(&bufferDesc, cbvHeap.cpuHandle(sceneHandle));
    pDevice->CreateDepthStencilView(pDepthStencil, &stencilDesc, dsvHeap.cpuHandle(depthHandle));

    CD3DX12_RANGE read(0, 0);
    HR_CHECK(pSceneBuffer->Map(0, &read, (void**)&pSceneData));
}

void ScenePass::stop() {
    auto& ctx = getContext();

    ctx.getDsvHeap().release(depthHandle);

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
    auto& dsvHeap = ctx.getDsvHeap();

    auto rtv = pRenderTargetOut->cpuHandle(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    auto dsv = dsvHeap.cpuHandle(depthHandle);

    Display display = createDisplay(info.renderResolution);

    cmd->RSSetViewports(1, &display.viewport);
    cmd->RSSetScissorRects(1, &display.scissor);

    cmd->SetGraphicsRootSignature(pRootSignature);
    cmd->SetPipelineState(pPipelineState);

    cmd->OMSetRenderTargets(1, &rtv, false, &dsv);
    cmd->ClearRenderTargetView(rtv, kClearColour, 0, nullptr);
    cmd->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, 1.f, 0, 0, nullptr);

    cmd->SetGraphicsRootDescriptorTable(0, cbvHeap.gpuHandle());
    cmd->SetGraphicsRootConstantBufferView(1, pSceneBuffer->GetGPUVirtualAddress());

    cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    if (rootNode != SIZE_MAX) {
        renderNode(rootNode, float4x4::identity());
    }
}

void ScenePass::renderNode(size_t idx, const float4x4& parent) {
    auto& ctx = getContext();
    auto cmd = ctx.getDirectCommands();
    const auto& node = nodes[idx];

    float4x4 transform = parent * node.asset.transform;
    node.pNodeData->transform = transform;

    cmd->SetGraphicsRootConstantBufferView(3, node.pResource->GetGPUVirtualAddress());

    for (const auto& primitive : node.asset.primitives) {
        const auto& prim = primitives[primitive];
        const auto& vertexBuffer = vertices[prim.vertexBuffer];
        const auto& indexBuffer = indices[prim.indexBuffer];

        cmd->SetGraphicsRoot32BitConstant(2, UINT32(prim.texture), 0);

        cmd->IASetVertexBuffers(0, 1, &vertexBuffer.view);
        cmd->IASetIndexBuffer(&indexBuffer.view);

        cmd->DrawIndexedInstanced(UINT(indexBuffer.size), 1, 0, 0, 0);
    }

    for (const auto& child : node.asset.children) {
        renderNode(child, transform);
    }
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
    return 0;
}

size_t UploadHandle::addVertexBuffer(std::span<const assets::Vertex> buffer) {
    auto& ctx = pSelf->getContext();
    auto pDevice = ctx.getDevice();

    auto direct = directCommands.pCommandList;
    auto copy = copyCommands.pCommandList;

    ID3D12Resource *pStagingBuffer = nullptr;
    ID3D12Resource *pVertexBuffer = nullptr;

    D3D12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(buffer.size_bytes());
    
    HR_CHECK(pDevice->CreateCommittedResource(
        &kUploadProps,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&pStagingBuffer)
    ));

    HR_CHECK(pDevice->CreateCommittedResource(
        &kDefaultProps,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(&pVertexBuffer)
    ));

    void *pStagingData = nullptr;
    HR_CHECK(pStagingBuffer->Map(0, nullptr, &pStagingData));
    memcpy(pStagingData, buffer.data(), buffer.size_bytes());
    pStagingBuffer->Unmap(0, nullptr);

    std::lock_guard guard(pSelf->mutex);

    copy->CopyResource(pVertexBuffer, pStagingBuffer);

    D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        pVertexBuffer,
        D3D12_RESOURCE_STATE_COMMON,
        D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER
    );

    direct->ResourceBarrier(1, &barrier);

    D3D12_VERTEX_BUFFER_VIEW bufferView = {
        .BufferLocation = pVertexBuffer->GetGPUVirtualAddress(),
        .SizeInBytes = UINT(buffer.size_bytes()),
        .StrideInBytes = sizeof(assets::Vertex)
    };

    ctx.submitCopyCommands(copyCommands);
    ctx.submitDirectCommands(directCommands);

    size_t result = pSelf->vertices.size();
    pSelf->vertices.push_back({ pVertexBuffer, bufferView });

    return result;
}

size_t UploadHandle::addIndexBuffer(std::span<const uint32_t> buffer) {
    auto& ctx = pSelf->getContext();
    auto pDevice = ctx.getDevice();
    
    auto direct = directCommands.pCommandList;
    auto copy = copyCommands.pCommandList;

    ID3D12Resource *pStagingBuffer = nullptr;
    ID3D12Resource *pIndexBuffer = nullptr;

    D3D12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(buffer.size_bytes());

    HR_CHECK(pDevice->CreateCommittedResource(
        &kUploadProps,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&pStagingBuffer)
    ));

    HR_CHECK(pDevice->CreateCommittedResource(
        &kDefaultProps,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(&pIndexBuffer)
    ));

    void *pStagingData = nullptr;
    HR_CHECK(pStagingBuffer->Map(0, nullptr, &pStagingData));
    memcpy(pStagingData, buffer.data(), buffer.size_bytes());
    pStagingBuffer->Unmap(0, nullptr);

    std::lock_guard guard(pSelf->mutex);

    copy->CopyResource(pIndexBuffer, pStagingBuffer);

    D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        pIndexBuffer,
        D3D12_RESOURCE_STATE_COMMON,
        D3D12_RESOURCE_STATE_INDEX_BUFFER
    );

    direct->ResourceBarrier(1, &barrier);

    D3D12_INDEX_BUFFER_VIEW bufferView = {
        .BufferLocation = pIndexBuffer->GetGPUVirtualAddress(),
        .SizeInBytes = UINT(buffer.size_bytes()),
        .Format = DXGI_FORMAT_R32_UINT
    };

    ctx.submitCopyCommands(copyCommands);
    ctx.submitDirectCommands(directCommands);

    size_t result = pSelf->indices.size();
    pSelf->indices.push_back({ pIndexBuffer, bufferView, UINT(buffer.size()) });
    
    return result;
}

size_t UploadHandle::addTexture(const assets::Texture& texture) {
    const auto& [data, size] = texture;

    auto& ctx = pSelf->getContext();
    auto pDevice = ctx.getDevice();
    auto& cbvHeap = ctx.getCbvHeap();
    auto& textures = pSelf->textures;

    auto direct = directCommands.pCommandList;
    auto copy = copyCommands.pCommandList;

    ID3D12Resource *pStagingTexture = nullptr;
    ID3D12Resource *pTexture = nullptr;

    D3D12_RESOURCE_DESC textureDesc = CD3DX12_RESOURCE_DESC::Tex2D(
        /* format = */ DXGI_FORMAT_R8G8B8A8_UNORM,
        /* width = */ UINT(size.x),
        /* height = */ UINT(size.y)
    );

    HR_CHECK(pDevice->CreateCommittedResource(
        &kDefaultProps,
        D3D12_HEAP_FLAG_NONE,
        &textureDesc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(&pTexture)
    ));

    UINT64 uploadSize = GetRequiredIntermediateSize(pTexture, 0, 1);
    D3D12_RESOURCE_DESC uploadDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadSize);

    HR_CHECK(pDevice->CreateCommittedResource(
        &kUploadProps,
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

    auto handle = cbvHeap.alloc();
    size_t result = textures.size();

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {
        .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
        .ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D,
        .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
        .Texture2D = { .MipLevels = 1 }
    };

    std::lock_guard guard(pSelf->mutex);

    UpdateSubresources(copy, pTexture, pStagingTexture, 0, 0, 1, &subresourceData);

    direct->ResourceBarrier(1, &barrier);
    
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

size_t UploadHandle::addPrimitive(const assets::Primitive& primitive) {
    auto& prims = pSelf->primitives;

    std::lock_guard guard(pSelf->mutex);
    size_t result = prims.size();
    prims.push_back(primitive);
    return result;
}

size_t UploadHandle::addNode(const assets::Node& node) {
    auto& ctx = pSelf->getContext();
    auto& cbvHeap = ctx.getCbvHeap();
    auto& nodes = pSelf->nodes;
    auto pDevice = ctx.getDevice();

    ID3D12Resource *pResource = ctx.newBuffer(
        sizeof(NodeBuffer),
        &kUploadProps,
        D3D12_RESOURCE_STATE_GENERIC_READ
    );
    
    auto handle = cbvHeap.alloc();

    Node it = {
        .asset = node,
        .pResource = pResource,
        .handle = handle
    };

    HR_CHECK(pResource->Map(0, nullptr, (void**)&it.pNodeData));
    it.pNodeData->transform = node.transform;

    D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {
        .BufferLocation = pResource->GetGPUVirtualAddress(),
        .SizeInBytes = sizeof(NodeBuffer)
    };

    pDevice->CreateConstantBufferView(&cbvDesc, cbvHeap.cpuHandle(handle));

    std::lock_guard guard(pSelf->mutex);
    size_t result = nodes.size();
    nodes.push_back(it);
    return result;
}

void UploadHandle::setNodeChildren(size_t idx, std::span<const size_t> children) {
    std::lock_guard guard(pSelf->mutex);
    std::copy(children.begin(), children.end(), std::back_inserter(pSelf->nodes[idx].asset.children));
}

void ScenePass::addUpload(const fs::path& path) {
    std::lock_guard guard(mutex);
    
    uploads.emplace_back(new UploadHandle(this, path));
}
