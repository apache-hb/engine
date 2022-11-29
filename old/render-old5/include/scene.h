#pragma once

#include "engine/render/render.h"
#include "engine/render/world.h"
#include "engine/render/data.h"

namespace simcoe::render {
    template<typename T>
    struct ConstBuffer {
        ConstBuffer() = default;

        ConstBuffer(rhi::Device& device, rhi::CpuHandle handle) {
            attach(device, handle);
        }

        void attach(rhi::Device& device, rhi::CpuHandle handle) {
            buffer = device.newBuffer(sizeof(T), rhi::DescriptorSet::Visibility::eHostVisible, rhi::Buffer::eUpload);
            device.createConstBufferView(buffer, sizeof(T), handle);
            ptr = buffer.map();
        }

        void update(const T& in) {
            data = in;
            upload();
        }

        void upload() {
            memcpy(ptr, &data, sizeof(T));
        }

        T& get() { return data; }
        const T& get() const { return data; }

    protected:
        T data;

    private:
        rhi::Buffer buffer;
        void *ptr;
    };

    struct DebugBufferHandle : ConstBuffer<DebugBuffer> {
        using Super = ConstBuffer<DebugBuffer>;
        using Super::Super;
    };

    struct SceneBufferHandle : ConstBuffer<SceneBuffer> {
        using Super = ConstBuffer<SceneBuffer>;
        using Super::Super;

        void update(Camera *camera, float aspectRatio, float3 light);
    };

    struct ObjectBufferHandle : ConstBuffer<ObjectBuffer> {
        using Super = ConstBuffer<ObjectBuffer>;
        using Super::Super;
    };

    struct BasicScene final : RenderScene {
        struct Create {
            Camera *camera;
            rhi::TextureSize resolution = { 1920, 1080 };
        };

        BasicScene(Create&& info);

        ID3D12CommandList *populate() override;
        ID3D12CommandList *attach(Context *ctx) override;

        size_t addTexture(const assets::Texture& texture);
        size_t addNode(const assets::Node& node);
        size_t addVertexBuffer(const assets::VertexBuffer& verts);
        size_t addIndexBuffer(const assets::IndexBuffer& indices);
        size_t addPrimitive(const assets::Primitive& primitive);

    private:
        template<typename F>
        auto update(F&& func) {
            std::lock_guard lock(mutex);
            return func(&world);
        }

        std::mutex mutex;
        assets::World world;

        Context *ctx;
        Camera *camera;

        void updateObject(size_t index, math::float4x4 parent);
        
        rhi::GpuHandle getObjectBufferGpuHandle(size_t index);
        rhi::CpuHandle getObjectBufferCpuHandle(size_t index);
        
        rhi::GpuHandle getTextureGpuHandle(size_t index);
        rhi::CpuHandle getTextureCpuHandle(size_t index);

        rhi::View view;
        float aspectRatio;

        size_t debugBufferOffset;
        size_t sceneBufferOffset;
        
        DebugBufferHandle debugData;
        SceneBufferHandle sceneData;

        std::vector<ObjectBufferHandle> objectData;

        UniquePtr<rhi::Allocator[]> allocators;

        rhi::DescriptorSet dsvHeap;
        rhi::Buffer depthBuffer;

        size_t objectBufferOffset;
        size_t textureBufferOffset;

        rhi::CommandList commands;

        std::vector<rhi::Buffer> vertexBuffers;
        std::vector<rhi::Buffer> indexBuffers;

        std::vector<rhi::VertexBufferView> vertexBufferViews;
        std::vector<rhi::IndexBufferView> indexBufferViews;

        float3 light = { 0.f, 0.f, 0.f };

        std::vector<rhi::Buffer> textures;
        
        rhi::PipelineState pso;
    };
}
