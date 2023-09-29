#pragma once

#include "simcoe/math/math.h"

#include "simcoe/simcoe.h"

#include <filesystem>
#include <fstream>
#include <span>

namespace simcoe::assets {
    namespace math = simcoe::math;

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

    struct Manager {
        Manager(const std::filesystem::path& root) : root(root) {}

        template<typename T>
        std::vector<T> loadBlob(const std::filesystem::path& path) {
            std::ifstream file(root / path, std::ios::binary);
            if (!file.is_open()) {
                gInputLog.warn("Failed to open file: {}", path.string());
                return {};
            }

            file.seekg(0, std::ios::end);
            size_t size = file.tellg();
            file.seekg(0, std::ios::beg);

            std::vector<T> data(size / sizeof(T));
            file.read(reinterpret_cast<char*>(data.data()), size);
            file.close();

            return data;
        }

        std::string loadText(const std::filesystem::path& path) {
            std::ifstream file(root / path);
            if (!file.is_open()) {
                gInputLog.warn("Failed to open file: {}", path.string());
                return {};
            }

            std::string data;
            file.seekg(0, std::ios::end);
            data.reserve(file.tellg());
            file.seekg(0, std::ios::beg);

            data.assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            file.close();

            return data;
        }

        std::shared_ptr<IUpload> gltf(const std::filesystem::path& path, IScene& scene);

    private:
        std::filesystem::path root;
    };
}
