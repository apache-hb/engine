#pragma once

#include "game/game.h"
#include "game/camera.h"

#include "simcoe/render/context.h"
#include "simcoe/render/graph.h"

#include "imgui/imgui.h"
#include "widgets/imfilebrowser.h"

#define CBUFFER alignas(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT)

namespace game {
    namespace render = simcoe::render;
    namespace os = simcoe::os;
    namespace math = simcoe::math;
    namespace util = simcoe::util;

    using ShaderBlob = std::vector<std::byte>;

    struct Vertex {
        math::float3 position;
        math::float2 uv;
    };

    struct CBUFFER SceneBuffer {
        math::float4x4 mvp;
    };

    struct CBUFFER NodeBuffer {
        math::float4x4 transform;
    };

    struct Display {
        D3D12_VIEWPORT viewport;
        D3D12_RECT scissor;
    };

    constexpr float kClearColour[] = { 0.0f, 0.2f, 0.4f, 1.0f };

    void uploadData(ID3D12Resource *pResource, size_t size, const void *pData);

    D3D12_SHADER_BYTECODE getShader(const ShaderBlob &blob);

    Display createDisplay(const os::Size& size);
    Display createLetterBoxDisplay(const os::Size& render, const os::Size& present);

    struct RenderEdge final : render::OutEdge {
        RenderEdge(const GraphObject& self, render::Pass *pPass);

        ID3D12Resource *getResource() override;

        D3D12_RESOURCE_STATES getState() const override;
        D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle(D3D12_DESCRIPTOR_HEAP_TYPE) override;

        D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle(D3D12_DESCRIPTOR_HEAP_TYPE) override;

        void start();
        void stop();

    private:
        struct FrameData {
            render::Heap::Index index;
            ID3D12Resource *pRenderTarget = nullptr;
        };

        std::vector<FrameData> frameData;
    };

    struct IntermediateTargetEdge final : render::OutEdge {
        IntermediateTargetEdge(const GraphObject& self, render::Pass *pPass, const os::Size& size);

        void start();
        void stop();

        ID3D12Resource *getResource() override;
        D3D12_RESOURCE_STATES getState() const override;
        D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle(D3D12_DESCRIPTOR_HEAP_TYPE type) override;
        D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle(D3D12_DESCRIPTOR_HEAP_TYPE type) override;

    private:
        os::Size size;

        D3D12_RESOURCE_STATES state;
        ID3D12Resource *pResource;

        render::Heap::Index cbvIndex = render::Heap::Index::eInvalid;
        render::Heap::Index rtvIndex = render::Heap::Index::eInvalid;
    };

    struct ScenePass;

    struct Mesh {
        ID3D12Resource *pVertexBuffer = nullptr;
        ID3D12Resource *pIndexBuffer = nullptr;

        D3D12_VERTEX_BUFFER_VIEW vertexView;
        D3D12_INDEX_BUFFER_VIEW indexView;
    };

    struct PipelineState {
        ID3D12PipelineState *pState = nullptr;
        ID3D12RootSignature *pRootSignature = nullptr;
    };

    struct Pass : render::Pass {
        Pass(const GraphObject& object, Info& info)
            : render::Pass(object)
            , info(info)
        { }

        Info& info;
    };

    struct GlobalPass final : Pass {
        GlobalPass(const GraphObject& object, Info& info);

        void start(ID3D12GraphicsCommandList*) override;
        void stop() override;

        void execute(ID3D12GraphicsCommandList*) override;

        RenderEdge *pRenderTargetOut = nullptr;
    };

    struct CubeMapPass final : Pass {
        CubeMapPass(const GraphObject& object, Info& info);

        void start(ID3D12GraphicsCommandList *pCommands) override;
        void stop() override;

        void execute(ID3D12GraphicsCommandList *pCommands) override;

        render::InEdge *pRenderTargetIn = nullptr;
        render::OutEdge *pRenderTargetOut = nullptr;

    private:
        ShaderBlob vs;
        ShaderBlob ps;

        Mesh cubeMapMesh;
        PipelineState cubeMapPSO;
    };

    struct ScenePass final : Pass {
        ScenePass(const GraphObject& object, Info& info);
        void start(ID3D12GraphicsCommandList *pCommands) override;
        void stop() override;

        void execute(ID3D12GraphicsCommandList *pCommands) override;

        IntermediateTargetEdge *pRenderTargetOut = nullptr;

    private:
        ShaderBlob vs;
        ShaderBlob ps;

        ID3D12RootSignature *pRootSignature = nullptr;
        ID3D12PipelineState *pPipelineState = nullptr;

        ID3D12Resource *pDepthStencil = nullptr;
        render::Heap::Index depthHandle = render::Heap::Index::eInvalid;

        ID3D12Resource *pSceneBuffer = nullptr;
        SceneBuffer *pSceneData = nullptr;
        render::Heap::Index sceneHandle = render::Heap::Index::eInvalid;
    };

    // copy resource to back buffer
    struct BlitPass final : Pass {
        BlitPass(const GraphObject& object, Info& info);

        void start(ID3D12GraphicsCommandList *pCommands) override;
        void stop() override;

        void execute(ID3D12GraphicsCommandList *pCommands) override;

        render::InEdge *pSceneTargetIn = nullptr;
        render::InEdge *pRenderTargetIn = nullptr;

        render::OutEdge *pSceneTargetOut = nullptr;
        render::OutEdge *pRenderTargetOut = nullptr;

    private:
        Display display;

        ShaderBlob ps;
        ShaderBlob vs;

        ID3D12RootSignature *pBlitSignature = nullptr;
        ID3D12PipelineState *pBlitPipeline = nullptr;

        // fullscreen quad
        ID3D12Resource *pVertexBuffer = nullptr;
        D3D12_VERTEX_BUFFER_VIEW vertexBufferView;

        ID3D12Resource *pIndexBuffer = nullptr;
        D3D12_INDEX_BUFFER_VIEW indexBufferView;

        std::unique_ptr<util::Entry> debug;
    };

    struct PresentPass final : Pass {
        PresentPass(const GraphObject& object, Info& info) : Pass(object, info) {
            pSceneTargetIn = in<render::InEdge>("scene-target", D3D12_RESOURCE_STATE_RENDER_TARGET);
            pRenderTargetIn = in<render::InEdge>("render-target", D3D12_RESOURCE_STATE_PRESENT);
        }

        void execute(ID3D12GraphicsCommandList*) override {
            // empty?
        }

        render::InEdge *pSceneTargetIn = nullptr;
        render::InEdge *pRenderTargetIn = nullptr;
    };

    struct ImGuiPass final : Pass {
        ImGuiPass(const GraphObject& object, Info& info);

        void start(ID3D12GraphicsCommandList *pCommands) override;
        void stop() override;

        void execute(ID3D12GraphicsCommandList *pCommands) override;

        render::InEdge *pRenderTargetIn = nullptr;
        render::OutEdge *pRenderTargetOut = nullptr;

    private:
        render::Heap::Index fontHandle = render::Heap::Index::eInvalid;

        void enableDock();
        void drawInfo();
        void drawInputInfo();

        ImGui::FileBrowser fileBrowser;
    };

    struct Scene final : render::Graph {
        Scene(render::Context& context, Info& info);

        void execute() {
            Graph::execute(pPresentPass);
        }

        void load(const std::filesystem::path& path);

        ScenePass& getScenePass() { return *pScenePass; }

    private:
        template<typename T, typename... A>
        T *newPass(std::string name, A&&... args) {
            return Graph::addPass<T>(name, info, std::forward<A>(args)...);
        }

        std::unique_ptr<util::Entry> debug;
        Info& info;

        GlobalPass *pGlobalPass;

        ScenePass *pScenePass;

        BlitPass *pBlitPass;

        ImGuiPass *pImGuiPass;
        PresentPass *pPresentPass;
    };
}
