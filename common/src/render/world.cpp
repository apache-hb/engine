#include "engine/render/world.h"

#include "engine/math/consts.h"

#include "engine/base/logging.h"
#include "engine/base/util.h"

#include <algorithm>
#include <unordered_map>

#include "tinygltf/tinygltf.h"

namespace gltf = tinygltf;

using namespace simcoe;
using namespace math;

template<typename T>
struct std::hash<math::Vec3<T>> {
    constexpr std::size_t operator()(const math::Vec3<T>& v) const noexcept {
        return std::hash<T>{}(v.x) ^ std::hash<T>{}(v.y) ^ std::hash<T>{}(v.z);
    }
};

template<typename T>
struct std::hash<math::Vec2<T>> {
    constexpr std::size_t operator()(const math::Vec2<T>& v) const noexcept {
        return std::hash<T>{}(v.x) ^ std::hash<T>{}(v.y);
    }
};

template<>
struct std::hash<assets::Vertex> {
    constexpr std::size_t operator()(const assets::Vertex& v) const noexcept {
        return std::hash<math::float3>{}(v.position) ^ std::hash<math::float3>{}(v.normal) ^ std::hash<math::float2>{}(v.uv);
    }
};

template<>
struct std::equal_to<assets::Vertex> {
    constexpr bool operator()(const assets::Vertex& lhs, const assets::Vertex& rhs) const noexcept {
        return lhs.position == rhs.position && lhs.normal == rhs.normal && lhs.uv == rhs.uv;
    }
};

namespace {
    math::float2 loadVec2(const float *data) {
        return { .x = data[0], .y = data[1] };
    }

    math::float3 loadVec3(const double *data) {
        return { .x = float(data[0]), .y = float(data[1]), .z = float(data[2]) };
    }

    math::float3 loadVertex(const float *data) {
        return { .x = data[0], .y = data[2], .z = data[1] };
    }

    math::float4 loadVec4(std::span<const double> data) {
        return { .x = float(data[0]), .y = float(data[1]), .z = float(data[2]), .w = float(data[3]) };
    }
    
    float4x4 createTransform(const gltf::Node& node) {
        auto result = float4x4::identity();

        if (node.matrix.size() == 16) {
            const auto &mat = node.matrix;
            result = math::float4x4::from(
                { float(mat[0]), float(mat[1]), float(mat[2]), float(mat[3]) },
                { float(mat[4]), float(mat[5]), float(mat[6]), float(mat[7]) },
                { float(mat[8]), float(mat[9]), float(mat[10]), float(mat[11]) },
                { float(mat[12]), float(mat[13]), float(mat[14]), float(mat[15]) }
            );
        }

        if (node.scale.size() == 3) {
            auto [x, y, z] = loadVec3(node.scale.data());
            result *= math::float4x4::scaling(x, y, z);
        }

        if (node.rotation.size() == 4) {
            auto rot = loadVec4(node.rotation);
            result *= math::float4x4::rotation(rot.w, rot.vec3());
        }

        if (node.translation.size() == 3) {
            auto [x, y, z] = loadVec3(node.translation.data());
            result *= math::float4x4::translation(x, y, z);
        }

        return result;
    }

    std::vector<size_t> loadChildren(std::span<const int> data) {
        std::vector<size_t> result;
        std::copy(data.begin(), data.end(), std::back_inserter(result));
        return result;
    }

    size_t getTexture(const gltf::Model& model, const gltf::Primitive& primitive) {
        if (primitive.material == -1) { return SIZE_MAX; }

        const auto& mat = model.materials[primitive.material];
        const auto& base = mat.pbrMetallicRoughness.baseColorTexture;

        if (base.index == -1) { return SIZE_MAX; }

        const auto& texture = model.textures[base.index];
        return texture.source;
    }

    struct AttributeData {
        const std::byte *data;
        size_t length;
        size_t stride;
        rhi::Format format;
    };

    rhi::Format formatFromComponent(int component, int type) {
        if (component == TINYGLTF_COMPONENT_TYPE_FLOAT) {
            switch (type) {
            case TINYGLTF_TYPE_VEC2: return rhi::Format::float32x2;
            case TINYGLTF_TYPE_VEC3: return rhi::Format::float32x3;
            default: break;
            }
        }
        
        ASSERTF(false, "unknown format (component: {}, type: {})", component, type);
    }

    AttributeData getAttribute(int index, const gltf::Model& model) {
        if (index == -1) { return { }; }

        const auto& accessor = model.accessors[index];
        const auto& bufferView = model.bufferViews[accessor.bufferView];
        const auto& buffer = model.buffers[bufferView.buffer];

        const auto* data = buffer.data.data() + accessor.byteOffset + bufferView.byteOffset;
        const auto stride = accessor.ByteStride(bufferView);

        return { 
            .data = reinterpret_cast<const std::byte*>(data), 
            .length = accessor.count, 
            .stride = size_t(stride),
            .format = formatFromComponent(accessor.componentType, accessor.type)
        };
    }

    constexpr uint32_t getComponentMask(int componentType) {
        switch (componentType) {
        case TINYGLTF_COMPONENT_TYPE_BYTE: return 0xFF;
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE: return 0xFF;
        case TINYGLTF_COMPONENT_TYPE_SHORT: return 0xFFFF;
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: return 0xFFFF;
        case TINYGLTF_COMPONENT_TYPE_INT: return 0xFFFFFFFF;
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT: return 0xFFFFFFFF;
        default: ASSERTF(false, "unknown component type: {}", componentType);
        }
    }

    constexpr uint8_t kDefaultImage[] = {
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    };

    struct Sink {
        Sink(const gltf::Model& model) 
            : model(model) 
        { }

        std::vector<size_t> getPrimitives(int index) {
            if (index == -1) { return { }; }
            const auto& mesh = model.meshes[index];

            std::vector<size_t> result;

            for (const auto& primitive : mesh.primitives) {
                auto texture = getTexture(model, primitive);

                auto position = getAttribute(primitive.attributes.at("POSITION"), model);
                auto uv = getAttribute(primitive.attributes.at("TEXCOORD_0"), model);
                auto normal = getAttribute(primitive.attributes.at("NORMAL"), model);

                ASSERT(position.format == rhi::Format::float32x3);                
                ASSERT(uv.format == rhi::Format::float32x2);

                auto data = getPrimitive(texture, position, uv, primitive.indices, normal);

                result.push_back(addPrimitive(data));
            }

            return result;
        }

        assets::IndexBuffer getIndexBuffer(int index) {
            if (index == -1) { return { }; }

            const auto& accessor = model.accessors[index];
            const auto& bufferView = model.bufferViews[accessor.bufferView];
            const auto& buffer = model.buffers[bufferView.buffer];

            const auto stride = accessor.ByteStride(bufferView);
            const auto size = accessor.count;

            // lol
            ASSERT(accessor.type == TINYGLTF_TYPE_SCALAR);

            auto mask = getComponentMask(accessor.componentType);

            // more lol
            assets::IndexBuffer indexBuffer(size);
            for (size_t i = 0; i < size; i++) {
                auto offset = accessor.byteOffset + bufferView.byteOffset + i * stride;
                auto data = reinterpret_cast<const std::byte*>(buffer.data.data() + offset);
                indexBuffer[i] = *reinterpret_cast<const uint32_t*>(data) & mask;
            }
            return indexBuffer;
        }

        assets::Primitive getPrimitive(size_t texture, const AttributeData& position, const AttributeData& uv, int indices, const AttributeData& normal) {
            std::unordered_map<assets::Vertex, uint32_t> indexCache;

            assets::VertexBuffer vertexBuffer;
            assets::IndexBuffer indexBuffer = getIndexBuffer(indices);

            for (size_t i = 0; i < position.length; i++) {
                auto pos = loadVertex((float*)(position.data + i * position.stride));
                auto coords = position.data == nullptr ? math::float2::of(0) : loadVec2((float*)(uv.data + i * uv.stride));
                auto nrm = normal.data == nullptr ? math::float3::of(0) : loadVertex((float*)(normal.data + i * normal.stride));

                assets::Vertex vertex { pos, nrm, coords };

                if (indices == -1) {
                    if (auto iter = indexCache.find(vertex); iter != indexCache.end()) {
                        indexBuffer.push_back(iter->second);
                    } else {
                        auto index = uint32_t(indexBuffer.size());

                        vertexBuffer.push_back(vertex);
                        indexBuffer.push_back(index);
                        indexCache[vertex] = index;
                    }
                } else {
                    vertexBuffer.push_back(vertex);
                }
            }

            if (normal.data == nullptr) {
                for (size_t i = 0; i < indexBuffer.size(); i += 3) {
                    auto a = vertexBuffer[indexBuffer[i + 0]].position;
                    auto b = vertexBuffer[indexBuffer[i + 1]].position;
                    auto c = vertexBuffer[indexBuffer[i + 2]].position;

                    auto n = float3::cross(b - a, c - a).normal();

                    vertexBuffer[indexBuffer[i + 0]].normal = n;
                    vertexBuffer[indexBuffer[i + 1]].normal = n;
                    vertexBuffer[indexBuffer[i + 2]].normal = n;
                }
            }

            logging::get(logging::eRender).info("primitive({}, {})", vertexBuffer.size(), indexBuffer.size());

            return { 
                .texture = texture == SIZE_MAX ? world.textures.size() - 1 : texture,
                .verts = addVertexBuffer(vertexBuffer),
                .indices = addIndexBuffer(indexBuffer)
            };
        }

        size_t addPrimitive(assets::Primitive prim) {
            size_t i = world.primitives.size();
            world.primitives.push_back(prim);
            return i;
        }

        size_t addVertexBuffer(assets::VertexBuffer buffer) {
            size_t i = world.verts.size();
            world.verts.push_back(buffer);
            return i;
        }

        size_t addIndexBuffer(assets::IndexBuffer buffer) {
            size_t i = world.indices.size();
            world.indices.push_back(buffer);
            return i;
        }

        assets::World world;
        const gltf::Model& model;
    };
}

assets::World assets::loadGltf(const char *path) {
    auto &channel = logging::get(logging::eRender);

    gltf::Model model;
    gltf::TinyGLTF loader;

    std::string err;
    std::string warn;

    auto result = loader.LoadASCIIFromFile(&model, &err, &warn, path);
    if (!warn.empty()) {
        channel.warn("{}", warn);
    }

    if (!err.empty()) {
        channel.fatal("{}", err);
    }

    if (!result) {
        return { };
    }

    Sink sink{model};

    for (const auto& image : model.images) {
        Texture it = {
            .name = image.name,
            .size = { size_t(image.width), size_t(image.height) }
        };

        it.data.resize(image.image.size());
        memcpy(it.data.data(), image.image.data(), it.data.size());

        sink.world.textures.push_back(it);
    }

    Texture defaultTexture = {
        .name = "default",
        .size = { 2, 2 },
        .data = { (std::byte*)kDefaultImage, (std::byte*)kDefaultImage + sizeof(kDefaultImage) }
    };
    sink.world.textures.push_back(defaultTexture);

    for (const auto& node : model.nodes) {
        Node it = {
            .name = node.name,
            .transform = createTransform(node),
            .children = loadChildren(node.children),
            .primitives = sink.getPrimitives(node.mesh)
        };

        sink.world.nodes.push_back(it);
    }

    channel.info("finished loading gltf `{}`", path);

    return sink.world;
}