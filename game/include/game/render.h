#pragma once

#include "game/game.h"

#include "simcoe/render/context.h"
#include "simcoe/render/graph.h"

namespace game {
    namespace math = simcoe::math;
    namespace render = simcoe::render;
    namespace system = simcoe::system;

    using ShaderBlob = std::vector<std::byte>;

    struct Vertex {
        math::float3 position;
        math::float2 uv;
    };

    struct Display {
        D3D12_VIEWPORT viewport;
        D3D12_RECT scissor;
    };

    ShaderBlob loadShader(std::string_view path);
    D3D12_SHADER_BYTECODE getShader(const ShaderBlob &blob);

    Display createDisplay(const system::Size& size);
    Display createLetterBoxDisplay(const system::Size& render, const system::Size& present);

    struct RenderEdge final : render::OutEdge {
        RenderEdge(const GraphObject& self, render::Pass *pPass);

        ID3D12Resource *getResource() override;

        D3D12_RESOURCE_STATES getState() const override;
        D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle() override;

        D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle() override;

        void init();
        void deinit();

    private:
        struct FrameData {
            render::Heap::Index index;
            ID3D12Resource *pRenderTarget = nullptr;
        };

        std::vector<FrameData> frameData;
    };

    struct SourceEdge final : render::OutEdge {
        SourceEdge(const GraphObject& self, render::Pass *pPass, D3D12_RESOURCE_STATES state);
        SourceEdge(const GraphObject& self, render::Pass *pPass, D3D12_RESOURCE_STATES state, ID3D12Resource *pResource, D3D12_CPU_DESCRIPTOR_HANDLE cpu, D3D12_GPU_DESCRIPTOR_HANDLE gpu);
        
        void setResource(ID3D12Resource *pOther, D3D12_CPU_DESCRIPTOR_HANDLE otherCpu, D3D12_GPU_DESCRIPTOR_HANDLE otherGpu);
        
        ID3D12Resource *getResource() override;
        D3D12_RESOURCE_STATES getState() const override;
        D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle() override;
        D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle() override;

    private:
        D3D12_RESOURCE_STATES state;
        ID3D12Resource *pResource;
        D3D12_CPU_DESCRIPTOR_HANDLE cpu;
        D3D12_GPU_DESCRIPTOR_HANDLE gpu;
    };

struct GlobalPass final : render::Pass {
    GlobalPass(const GraphObject& object) : Pass(object) { 
        pRenderTargetOut = out<RenderEdge>("render-target");
    }

    void start() override {
        pRenderTargetOut->init();
    }

    void stop() override {
        pRenderTargetOut->deinit();
    }

    void execute() override { }

    RenderEdge *pRenderTargetOut = nullptr;
};

struct ScenePass final : render::Pass {
    ScenePass(const GraphObject& object, const system::Size& size) 
        : Pass(object)
        , size(size)
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
            /* width = */ UINT(size.width),
            /* height = */ UINT(size.height),
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
        Display display = createDisplay(size);

        cmd->RSSetViewports(1, &display.viewport);
        cmd->RSSetScissorRects(1, &display.scissor);

        cmd->OMSetRenderTargets(1, &rtv, false, nullptr);
        cmd->ClearRenderTargetView(rtv, kClearColour, 0, nullptr);
    }

    SourceEdge *pRenderTargetOut = nullptr;

private:
    system::Size size;
    
    constexpr static float kClearColour[] = { 0.0f, 0.2f, 0.4f, 1.0f };

    // TODO: this should be part of SourceEdge maybe?
    render::Heap::Index cbvIndex = render::Heap::Index::eInvalid;
    render::Heap::Index rtvIndex = render::Heap::Index::eInvalid;
};

// copy resource to back buffer
struct BlitPass final : render::Pass {
    BlitPass(const GraphObject& object, const Display& display) 
        : Pass(object) 
        , display(display)
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
            D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
            nullptr,
            IID_PPV_ARGS(&pVertexBuffer)
        ));

        HR_CHECK(device->CreateCommittedResource(
            &props,
            D3D12_HEAP_FLAG_NONE,
            &indexBufferDesc,
            D3D12_RESOURCE_STATE_INDEX_BUFFER,
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
        
        cmd->RSSetViewports(1, &display.viewport);
        cmd->RSSetScissorRects(1, &display.scissor);
        
        cmd->OMSetRenderTargets(1, &rtv, false, nullptr);
        cmd->ClearRenderTargetView(rtv, kClearColor, 0, nullptr);

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
    Display display;

    constexpr static float kClearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };

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

struct ImGuiPass final : render::Pass {
    ImGuiPass(const GraphObject& object, Info& info, input::Manager& manager);
    
    void start() override;
    void stop() override;

    void execute() override;

    render::InEdge *pRenderTargetIn = nullptr;
    render::OutEdge *pRenderTargetOut = nullptr;

private:
    Info& info;
    input::Manager& inputManager;
    render::Heap::Index fontHandle = render::Heap::Index::eInvalid;
    
    void enableDock();
    void drawInfo();
    void drawRenderGraphInfo();
    void drawGdkInfo();
    void drawInputInfo();

    struct Link {
        int src;
        int dst;
    };

    std::unordered_map<render::Pass*, int> passIndices;
    std::unordered_map<render::Edge*, int> edgeIndices;
    std::unordered_map<int, Link> links;
};

struct Scene final : render::Graph {
    Scene(render::Context& context, Info& info, input::Manager& input) : render::Graph(context) {
        Display present = createLetterBoxDisplay(info.renderResolution, info.windowResolution);

        pGlobalPass = addPass<GlobalPass>("global");
        
        pScenePass = addPass<ScenePass>("scene", info.renderResolution);
        pBlitPass = addPass<BlitPass>("blit", present);

        pImGuiPass = addPass<ImGuiPass>("imgui", info, input);
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

}
