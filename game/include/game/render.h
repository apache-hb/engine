#pragma once

#include "game/game.h"
#include "game/camera.h"

#include "simcoe/render/context.h"
#include "simcoe/render/graph.h"

#include "imgui/imgui.h"
#include "widgets/imfilebrowser.h"

#include <fastgltf/fastgltf_types.hpp>
#include <span>

#define CBUFFER alignas(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT)

namespace game {
    namespace render = simcoe::render;
    namespace system = simcoe::system;
    namespace math = simcoe::math;

    using ShaderBlob = std::vector<std::byte>;

    struct Vertex {
        math::float3 position;
        math::float2 uv;
    };

    struct CBUFFER SceneBuffer {
        math::float4x4 mvp;
    };

    struct CBUFFER MaterialBuffer {
        UINT texture;
    };

    struct Display {
        D3D12_VIEWPORT viewport;
        D3D12_RECT scissor;
    };

    constexpr float kClearColour[] = { 0.0f, 0.2f, 0.4f, 1.0f };

    ShaderBlob loadShader(std::string_view path);
    D3D12_SHADER_BYTECODE getShader(const ShaderBlob &blob);

    Display createDisplay(const system::Size& size);
    Display createLetterBoxDisplay(const system::Size& render, const system::Size& present);

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
        IntermediateTargetEdge(const GraphObject& self, render::Pass *pPass, const system::Size& size);

        void start();
        void stop();

        ID3D12Resource *getResource() override;
        D3D12_RESOURCE_STATES getState() const override;
        D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle(D3D12_DESCRIPTOR_HEAP_TYPE type) override;
        D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle(D3D12_DESCRIPTOR_HEAP_TYPE type) override;

    private:
        system::Size size;

        D3D12_RESOURCE_STATES state;
        ID3D12Resource *pResource;

        render::Heap::Index cbvIndex = render::Heap::Index::eInvalid;
        render::Heap::Index rtvIndex = render::Heap::Index::eInvalid;
    };

    struct GlobalPass final : render::Pass {
        GlobalPass(const GraphObject& object) : Pass(object) { 
            pRenderTargetOut = out<RenderEdge>("render-target");
        }

        void start() override {
            pRenderTargetOut->start();
        }

        void stop() override {
            pRenderTargetOut->stop();
        }

        void execute() override { }

        RenderEdge *pRenderTargetOut = nullptr;
    };

    struct ScenePass final : render::Pass {
        using AssetPtr = std::unique_ptr<fastgltf::Asset>;
        ScenePass(const GraphObject& object, Info& info);
        void start() override;
        void stop() override;

        void execute() override;

        void load(AssetPtr asset);

        IntermediateTargetEdge *pRenderTargetOut = nullptr;

    private:
        void uploadBuffer(std::span<const uint8_t> data);

        Info& info;

        AssetPtr scene;

        ShaderBlob vs;
        ShaderBlob ps;

        ID3D12RootSignature *pRootSignature = nullptr;
        ID3D12PipelineState *pPipelineState = nullptr;

        ID3D12Resource *pSceneBuffer = nullptr;
        SceneBuffer *pSceneData = nullptr;
        render::Heap::Index sceneHandle = render::Heap::Index::eInvalid;

        struct ObjectBuffer {
            ID3D12Resource *pVertexBuffer = nullptr;
            D3D12_VERTEX_BUFFER_VIEW vertexBufferView;

            ID3D12Resource *pIndexBuffer = nullptr;
            D3D12_INDEX_BUFFER_VIEW indexBufferView;

            size_t textureIndex = 0;
        };

        struct TextureBuffer {
            ID3D12Resource *pTexture = nullptr;
            render::Heap::Index handle = render::Heap::Index::eInvalid;
        };

        std::vector<ObjectBuffer> objectBuffers;
        std::vector<TextureBuffer> textureBuffers;
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

        ImGui::FileBrowser fileBrowser;
    };

    struct Scene final : render::Graph {
        Scene(render::Context& context, Info& info, input::Manager& input) : render::Graph(context) {
            Display present = createLetterBoxDisplay(info.renderResolution, info.windowResolution);

            pGlobalPass = addPass<GlobalPass>("global");
            
            pScenePass = addPass<ScenePass>("scene", info);
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

        void load(const std::filesystem::path& path);

    private:
        GlobalPass *pGlobalPass;

        ScenePass *pScenePass;
        BlitPass *pBlitPass;

        ImGuiPass *pImGuiPass;
        PresentPass *pPresentPass;
    };
}
