#pragma once

#include "simcoe/math/math.h"

#include <filesystem>
#include <span>
#include <vector>

namespace simcoe::assets {
    namespace fs = std::filesystem;
    struct Vertex {
        math::float3 position;
        math::float2 uv;
    };

    struct Texture {
        const uint8_t *pData;
        math::size2 size;
    };

    struct Primitive {
        size_t vertexBuffer;
        size_t indexBuffer;
        size_t texture;
    };

    struct Node {
        math::float4x4 transform;

        std::vector<size_t> children;
        std::vector<size_t> primitives;
    };

    struct IScene {
        virtual ~IScene() = default;

        virtual size_t getDefaultTexture() = 0;

        virtual size_t addVertexBuffer(std::span<const Vertex> data) = 0;
        virtual size_t addIndexBuffer(std::span<const uint32_t> data) = 0;
        virtual size_t addTexture(const Texture& texture) = 0;
        virtual size_t addPrimitive(const Primitive& primitive) = 0;
        virtual size_t addNode(const Node& node) = 0;

        virtual void setNodeChildren(size_t node, std::span<const size_t> children) = 0;
        
        virtual void beginUpload() = 0;
        virtual void endUpload() = 0;
    };

    struct IUpload {
        virtual ~IUpload() = default;

        virtual bool isDone() const = 0;
        virtual float getProgress() const = 0;
    };

    std::shared_ptr<IUpload> gltf(const fs::path& path, IScene& scene);
}
