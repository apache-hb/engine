#include "game/render.h"
#include "game/registry.h"

using namespace game;
using namespace simcoe;

namespace {
    const D3D12_HEAP_PROPERTIES kUploadProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    const D3D12_HEAP_PROPERTIES kDefaultProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

    constexpr const char *stateToString(ModelPass::State state) {
        switch (state) {
        case ModelPass::ePending: return "Pending";
        case ModelPass::eWorking: return "Working";
        case ModelPass::eReady: return "Ready";
        default: return "Unknown";
        }
    }
}

ModelPass::ModelPass(const GraphObject& object, Info& info, const fs::path& path) 
    : Pass(object, info)
    , name(path.filename().string())
{
    auto& ctx = getContext();

    copyCommands = ctx.newCommandBuffer(D3D12_COMMAND_LIST_TYPE_COPY);
    directCommands = ctx.newCommandBuffer(D3D12_COMMAND_LIST_TYPE_DIRECT);

    upload = info.assets.gltf(path, *this);

    debug = game::debug.newEntry({ name.c_str() }, [this] {
        ImGui::Text("State: %s", stateToString(state));
        
        ImGui::Text("Nodes: %zu", nodes.size());
        ImGui::Text("Vertices: %zu", vertices.size());
        ImGui::Text("Indices: %zu", indices.size());
        ImGui::Text("Textures: %zu", textures.size());

        auto& ctx = getContext();
        auto& cbvHeap = ctx.getCbvHeap();

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

void ModelPass::start(ID3D12GraphicsCommandList*) {

}

void ModelPass::stop() {

}

void ModelPass::execute(ID3D12GraphicsCommandList*) {

}

size_t ModelPass::getDefaultTexture() {
    ASSERT(state == eWorking);
    return 0;
}

size_t ModelPass::addVertexBuffer(std::span<const assets::Vertex> buffer) {
    ASSERT(state == eWorking);
    auto& ctx = getContext();
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

    size_t result = vertices.size();
    vertices.push_back({ pVertexBuffer, bufferView });

    return result;
}

size_t ModelPass::addIndexBuffer(std::span<const uint32_t> buffer) {
    ASSERT(state == eWorking);
    auto& ctx = getContext();
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

    size_t result = indices.size();
    indices.push_back({ pIndexBuffer, bufferView, UINT(buffer.size()) });
    
    return result;
}

size_t ModelPass::addTexture(const assets::Texture& texture) {
    ASSERT(state == eWorking);
    const auto& [data, size] = texture;

    auto& ctx = getContext();
    auto pDevice = ctx.getDevice();
    auto& cbvHeap = ctx.getCbvHeap();

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

size_t ModelPass::addPrimitive(const assets::Primitive& primitive) {
    ASSERT(state == eWorking);
    size_t result = primitives.size();
    primitives.push_back(primitive);
    return result;
}

size_t ModelPass::addNode(const assets::Node& node) {
    ASSERT(state == eWorking);
    auto& ctx = getContext();
    auto& cbvHeap = ctx.getCbvHeap();
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

    size_t result = nodes.size();
    nodes.push_back(it);
    return result;
}

void ModelPass::setNodeChildren(size_t idx, std::span<const size_t> children) {
    ASSERT(state == eWorking);
    std::copy(children.begin(), children.end(), std::back_inserter(nodes[idx].asset.children));
}
