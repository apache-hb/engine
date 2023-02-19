#include "game/render.h"

using namespace game;
using namespace simcoe;

namespace {
    constexpr Vertex kCubeVertices[] = {
        { { -1.0f, -1.0f, -1.0f }, { 0.0f, 0.0f } },
        { { -1.0f, -1.0f,  1.0f }, { 0.0f, 1.0f } },
        { { -1.0f,  1.0f, -1.0f }, { 1.0f, 0.0f } },
        { { -1.0f,  1.0f,  1.0f }, { 1.0f, 1.0f } },
        { {  1.0f, -1.0f, -1.0f }, { 0.0f, 0.0f } },
        { {  1.0f, -1.0f,  1.0f }, { 0.0f, 1.0f } },
        { {  1.0f,  1.0f, -1.0f }, { 1.0f, 0.0f } },
        { {  1.0f,  1.0f,  1.0f }, { 1.0f, 1.0f } }
    };
    
    constexpr UINT kCubeIndices[] = {
        0, 1, 2, 3, 2, 1,
        4, 6, 5, 5, 6, 7,
        0, 2, 4, 6, 4, 2,
        1, 5, 3, 7, 3, 5,
        0, 4, 1, 5, 1, 4,
        2, 3, 6, 7, 6, 3
    };
}

ScenePass::ScenePass(const GraphObject& object, Info& info) 
    : Pass(object)
    , info(info)
{
    pRenderTargetOut = out<IntermediateTargetEdge>("scene-target", info.renderResolution);

    vs = loadShader("build\\game\\game.exe.p\\scene.vs.cso");
    ps = loadShader("build\\game\\game.exe.p\\scene.ps.cso");
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
    rootSignatureDesc.Init(_countof(rootParameters), (D3D12_ROOT_PARAMETER*)rootParameters, _countof(samplers), samplers, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

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
        .InputLayout = { layout, _countof(layout) },
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

    // upload vertices and indices

    D3D12_RESOURCE_DESC vertexBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(Vertex) * _countof(kCubeVertices));
    D3D12_RESOURCE_DESC indexBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(uint16_t) * _countof(kCubeIndices));

    HR_CHECK(pDevice->CreateCommittedResource(
        &props, 
        D3D12_HEAP_FLAG_NONE, 
        &vertexBufferDesc, 
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr, 
        IID_PPV_ARGS(&pVertexBuffer)
    ));

    HR_CHECK(pDevice->CreateCommittedResource(
        &props, 
        D3D12_HEAP_FLAG_NONE, 
        &indexBufferDesc, 
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr, 
        IID_PPV_ARGS(&pIndexBuffer)
    ));

    CD3DX12_RANGE readRange(0, 0);
    Vertex* pVertexData;
    uint16_t* pIndexData;

    HR_CHECK(pVertexBuffer->Map(0, &readRange, (void**)&pVertexData));
    memcpy(pVertexData, kCubeVertices, sizeof(kCubeVertices));

    HR_CHECK(pIndexBuffer->Map(0, &readRange, (void**)&pIndexData));
    memcpy(pIndexData, kCubeIndices, sizeof(kCubeIndices));

    pVertexBuffer->Unmap(0, nullptr);
    pIndexBuffer->Unmap(0, nullptr);

    vertexBufferView = {
        .BufferLocation = pVertexBuffer->GetGPUVirtualAddress(),
        .SizeInBytes = sizeof(Vertex) * _countof(kCubeVertices),
        .StrideInBytes = sizeof(Vertex)
    };
    
    indexBufferView = {
        .BufferLocation = pIndexBuffer->GetGPUVirtualAddress(),
        .SizeInBytes = sizeof(uint16_t) * _countof(kCubeIndices),
        .Format = DXGI_FORMAT_R16_UINT
    };

    // generate and upload 2x2 black texture
    std::vector<uint8_t> blackTexture(4 * 4 * 4, 0);
    D3D12_RESOURCE_DESC textureDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, 4, 4, 1, 1);
    D3D12_HEAP_PROPERTIES defaultProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    HR_CHECK(pDevice->CreateCommittedResource(
        &defaultProps, 
        D3D12_HEAP_FLAG_NONE, 
        &textureDesc, 
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr, 
        IID_PPV_ARGS(&pTexture)
    ));

    // create staging resource
    D3D12_RESOURCE_DESC stagingDesc = CD3DX12_RESOURCE_DESC::Buffer(blackTexture.size());
    ID3D12Resource *pStagingTexture = nullptr;
    HR_CHECK(pDevice->CreateCommittedResource(
        &props, 
        D3D12_HEAP_FLAG_NONE, 
        &stagingDesc, 
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr, 
        IID_PPV_ARGS(&pStagingTexture)
    ));

    uint8_t* pTextureData;
    HR_CHECK(pStagingTexture->Map(0, &readRange, (void**)&pTextureData));
    memcpy(pTextureData, blackTexture.data(), blackTexture.size());
    pStagingTexture->Unmap(0, nullptr);

    // copy staging resource to default resource
    // TODO:

    textureHandle = cbvHeap.alloc();

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {
        .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
        .ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D,
        .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
        .Texture2D = {
            .MipLevels = 1
        }
    };

    pDevice->CreateShaderResourceView(pTexture, &srvDesc, cbvHeap.cpuHandle(textureHandle));
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
    auto cmd = ctx.getCommandList();
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
    cmd->SetGraphicsRoot32BitConstant(2, UINT32(textureHandle), 0);
}
