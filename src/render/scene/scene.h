#pragma once

#include "render/objects/resource.h"
#include "render/objects/library.h"
#include "render/objects/device.h"
#include "render/camera/camera.h"

namespace engine::render {
    struct Context;

    namespace SceneData {
        enum Index : int {
            Camera,
            DefaultTexture,
            Total
        };
    }

    struct Scene {
        Scene(Context* context): ctx(context) { }

        virtual ~Scene() = default;

        virtual void create() = 0;
        virtual void destroy() = 0;
        virtual ID3D12CommandList* populate() = 0;

    protected:
        Context* ctx;
    };

    struct Scene3D : Scene {
        Scene3D(Context* context, Camera* camera);
        virtual ~Scene3D() = default;

        virtual void create() override;
        virtual void destroy() override;

        virtual ID3D12CommandList* populate() override;

    private:
        Camera* camera;

        void begin();
        void end();

        Device<ID3D12Device4>& getDevice();
        CD3DX12_CPU_DESCRIPTOR_HANDLE cbvSrvCpuHandle(UINT index);
        CD3DX12_GPU_DESCRIPTOR_HANDLE cbvSrvGpuHandle(UINT index);

        void createCommandList();
        void destroyCommandList();

        void createDsvHeap();
        void destroyDsvHeap();

        void createCbvSrvHeap();
        void destroyCbvSrvHeap();

        void createCameraBuffer();
        void destroyCameraBuffer();

        void createDepthStencil();
        void destroyDepthStencil();

        void createRootSignature();
        void destroyRootSignature();

        void createPipelineState();
        void destroyPipelineState();

        UINT getRequiredCbvSrvSize() const;

        CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle();

        Object<ID3D12DescriptorHeap> dsvHeap;
        Resource depthStencil;

        DescriptorHeap cbvSrvHeap;

    public:
        CameraBuffer cameraBuffer;
        CameraBuffer *cameraData;
        Resource cameraResource;

        Resource defaultTexture;

        VertexBuffer cubeVertices;
        IndexBuffer cubeIndices;

        ShaderLibrary shaders;
        Object<ID3D12GraphicsCommandList> commandList;
        Object<ID3D12RootSignature> rootSignature;
        Object<ID3D12PipelineState> pipelineState;
    };
}
