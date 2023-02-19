#pragma once

#include "game/game.h"

#include "simcoe/render/context.h"
#include "simcoe/render/graph.h"

namespace game {
    namespace render = simcoe::render;
    namespace system = simcoe::system;

    using ShaderBlob = std::vector<std::byte>;

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
        ScenePass(const GraphObject& object, const system::Size& size);
        void start() override;
        void stop() override;

        void execute() override;

        SourceEdge *pRenderTargetOut = nullptr;

    private:
        system::Size size;
        
        // TODO: this should be part of SourceEdge maybe?
        render::Heap::Index cbvIndex = render::Heap::Index::eInvalid;
        render::Heap::Index rtvIndex = render::Heap::Index::eInvalid;
    };

    // copy resource to back buffer
    struct BlitPass final : render::Pass {
        BlitPass(const GraphObject& object, const Display& display);

        void start() override;
        void stop() override;

        void execute() override;

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
