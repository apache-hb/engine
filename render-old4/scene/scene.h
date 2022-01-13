#pragma once

#include "render/objects/resource.h"
#include "render/objects/library.h"
#include "render/objects/device.h"
#include "render/camera/camera.h"

#include "assets/world.h"

namespace engine::render {
    struct Context;

    namespace SceneData {
        enum Index : int {
            Camera,
            Total
        };
    }

    struct Scene {
        Scene(Context* context): ctx(context) { }

        virtual ~Scene() = default;

        virtual void create() = 0;
        virtual void destroy() = 0;
        virtual ID3D12CommandList* populate() = 0;

    public:
        Device<ID3D12Device4>& getDevice();

        void createCommandList();
        void destroyCommandList();

        Object<ID3D12GraphicsCommandList> commandList;

        Context* ctx;
    };

    struct Scene3D : Scene {
        Scene3D(Context* context, Camera* camera, const assets::World* world);
        virtual ~Scene3D() = default;

        virtual void create() override;
        virtual void destroy() override;
        void changeScene(const assets::World* world);

        virtual ID3D12CommandList* populate() override;

    private:
        void createScene();
        void destroyScene();

        Camera* camera;
        const assets::World* world;

        void begin();
        void end();

        CD3DX12_CPU_DESCRIPTOR_HANDLE cbvSrvCpuHandle(UINT index);
        CD3DX12_GPU_DESCRIPTOR_HANDLE cbvSrvGpuHandle(UINT index);

        CD3DX12_CPU_DESCRIPTOR_HANDLE nodeCpuHandle(UINT i);

        void createDsvHeap();
        void destroyDsvHeap();

        void createCbvSrvHeap();
        void destroyCbvSrvHeap();

        void createConstBuffers();
        void destroyConstBuffers();

        void createDepthStencil();
        void destroyDepthStencil();

        void createRootSignature();
        void destroyRootSignature();

        void createPipelineState();
        void destroyPipelineState();

        void createSceneData();
        void destroySceneData();

        UINT getRequiredCbvSrvSize() const;

        CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle();

        Object<ID3D12DescriptorHeap> dsvHeap;
        Resource depthStencil;

        DescriptorHeap cbvSrvHeap;

    public: 
        struct ModelTransform {
            float4x4 model;
            CameraBuffer* data;
            Resource resource;
        };

        std::vector<VertexBuffer> vertexBuffers;
        std::vector<IndexBuffer> indexBuffers;
        std::vector<Resource> textures;
        std::vector<ModelTransform> transforms;

        ShaderLibrary shaders;
        Object<ID3D12RootSignature> rootSignature;
        Object<ID3D12PipelineState> pipelineState;
    };
}
