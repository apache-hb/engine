#pragma once

#include "game/game.h"
#include "game/camera.h"

#include "simcoe/render/context.h"
#include "simcoe/render/graph.h"

#include "simcoe/assets/assets.h"

#include "imgui/imgui.h"
#include "widgets/imfilebrowser.h"

#define CBUFFER alignas(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT)

namespace game {
    namespace render = simcoe::render;
    namespace system = simcoe::system;
    namespace assets = simcoe::assets;
    namespace math = simcoe::math;
    namespace util = simcoe::util;
    namespace fs = std::filesystem;

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

    struct ScenePass;

    struct Node {
        assets::Node asset;
        ID3D12Resource *pResource = nullptr;
        NodeBuffer *pNodeData = nullptr;
        render::Heap::Index handle = render::Heap::Index::eInvalid;
    };

    struct UploadHandle : assets::IScene {
        friend ScenePass;

        UploadHandle(ScenePass *pSelf, const fs::path& path);

        size_t getDefaultTexture() override;

        size_t addVertexBuffer(std::span<const assets::Vertex> data) override;
        size_t addIndexBuffer(std::span<const uint32_t> data) override;
        
        size_t addTexture(const assets::Texture& texture) override;

        size_t addPrimitive(const assets::Primitive& mesh) override;
        size_t addNode(const assets::Node& node) override;

        void setNodeChildren(size_t idx, std::span<const size_t> children) override;

        std::string name;
        std::shared_ptr<assets::IUpload> upload;
        std::unique_ptr<util::Entry> debug;

    private:
        ScenePass *pSelf;

        render::CommandBuffer copyCommands;
        render::CommandBuffer directCommands;
    };

    struct ScenePass final : render::Pass {
        friend UploadHandle;
        
        ScenePass(const GraphObject& object, Info& info);
        void start() override;
        void stop() override;

        void execute() override;

        IntermediateTargetEdge *pRenderTargetOut = nullptr;

        void addUpload(const fs::path& name);

    private:
        Info& info;

        ShaderBlob vs;
        ShaderBlob ps;

        ID3D12RootSignature *pRootSignature = nullptr;
        ID3D12PipelineState *pPipelineState = nullptr;

        ID3D12Resource *pDepthStencil = nullptr;
        render::Heap::Index depthHandle = render::Heap::Index::eInvalid;

        ID3D12Resource *pSceneBuffer = nullptr;
        SceneBuffer *pSceneData = nullptr;
        render::Heap::Index sceneHandle = render::Heap::Index::eInvalid;

        struct TextureHandle {
            std::string name;
            math::size2 size;
            ID3D12Resource *pResource = nullptr;
            render::Heap::Index handle = render::Heap::Index::eInvalid;
        };

        struct IndexBuffer {
            ID3D12Resource *pResource = nullptr;
            D3D12_INDEX_BUFFER_VIEW view;
            UINT size;
        };
        
        struct VertexBuffer {
            ID3D12Resource *pResource = nullptr;
            D3D12_VERTEX_BUFFER_VIEW view;
        };

        void renderNode(size_t idx, const float4x4& parent);

        std::unique_ptr<util::Entry> debug;

        std::mutex mutex;
        std::vector<std::unique_ptr<UploadHandle>> uploads;

        std::vector<assets::Primitive> primitives;
        std::vector<Node> nodes;

        std::vector<TextureHandle> textures;
        std::vector<IndexBuffer> indices;
        std::vector<VertexBuffer> vertices;

        size_t rootNode = SIZE_MAX;
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

        std::unique_ptr<util::Entry> debug;
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
        void drawInputInfo();

        ImGui::FileBrowser fileBrowser;
    };

    struct Scene final : render::Graph {
        Scene(render::Context& context, Info& info, input::Manager& input);

        void execute() {
            Graph::execute(pPresentPass);
        }

        void load(const std::filesystem::path& path);

        ScenePass& getScenePass() { return *pScenePass; }

    private:
        std::unique_ptr<util::Entry> debug;

        GlobalPass *pGlobalPass;

        ScenePass *pScenePass;
        BlitPass *pBlitPass;

        ImGuiPass *pImGuiPass;
        PresentPass *pPresentPass;
    };
}
