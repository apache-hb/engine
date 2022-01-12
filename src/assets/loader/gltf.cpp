#include "assets/loader.h"
#include "logging/log.h"
#include "gltf/tinygltf.h"
#include "debug.h"

#include <unordered_map>

template<>
struct std::hash<engine::math::float3> {
    std::size_t operator()(const engine::math::float3& v) const noexcept {
        return std::hash<float>{}(v.x) ^ std::hash<float>{}(v.y) ^ std::hash<float>{}(v.z);
    }
};

template<>
struct std::equal_to<engine::math::float3> {
    bool operator()(const engine::math::float3& lhs, const engine::math::float3& rhs) const noexcept {
        return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
    }
};

namespace engine::loader {
    using namespace engine::math;
    namespace gltf = tinygltf;

    gltf::TinyGLTF loader;

    // gltf operates with +y up, we use +z up
    // so we need to flip the y and z axis
    float3 getVec3(const uint8_t* data) {
        const float3* vec = reinterpret_cast<const float3*>(data);
        return float3::from(vec->x, vec->z, vec->y);
    }

    float2 getVec2(const uint8_t* data) {
        const float2* vec = reinterpret_cast<const float2*>(data);
        return { vec->x, vec->y };
    }

    struct AttributeData {
        const uint8_t* data;
        size_t length;
        int stride;
    };

    struct MeshData {
        std::vector<assets::Vertex> vertices;
        std::vector<uint32_t> indices;
        assets::Mesh mesh;
    };

    struct DataSink {
        std::unordered_map<float3, uint32_t> unique;

        std::vector<assets::Vertex> vertices;
        std::vector<uint32_t> indices;
    };

    assets::Mesh getVec3Array(const AttributeData& position, const AttributeData& texcoord, DataSink* sink, bool genIndices) {
        size_t indexStart = sink->indices.size();

        for (size_t i = 0; i < position.length; i++) {
            auto vec = getVec3(position.data + i * position.stride);
            auto coords = position.data == nullptr ? float2 { 0.f, 0.f } 
                : getVec2(texcoord.data + i * texcoord.stride);

            if (genIndices) {
                if (auto iter = sink->unique.find(vec); iter != sink->unique.end()) {
                    sink->indices.push_back(iter->second);
                } else {
                    auto index = uint32_t(sink->vertices.size());

                    sink->vertices.push_back({ vec, coords });
                    sink->indices.push_back(index);
                    sink->unique[vec] = index;
                }
            } else {
                sink->vertices.push_back({ vec, coords });
            }
        }

        assets::IndexBufferView view = { .offset = indexStart, .length = sink->indices.size() - indexStart };

        assets::Mesh mesh = {
            .views = { view },
            .buffer = 0,
            .material = { 0, 0 }
        };

        return mesh;
    }

    assets::IndexBufferView getIndexArray(const gltf::Model& model, DataSink* sink, int index) {
        const auto& accessor = model.accessors[index];
        const auto& bufferView = model.bufferViews[accessor.bufferView];
        const auto& buffer = model.buffers[bufferView.buffer];

        const auto data = buffer.data.data() + accessor.byteOffset + bufferView.byteOffset;
        const auto stride = accessor.ByteStride(bufferView);
        const auto elements = accessor.count;

        size_t start = sink->indices.size();
        sink->indices.resize(start + elements);

#define COPY_INDICES(type) \
            for (size_t i = 0; i < elements; i++) { \
                sink->indices[i + start] = *reinterpret_cast<const type*>(data + i * stride); \
            } \
            break;

        /**
         * i hate the antichrist (whoever wrote the gltf 2 spec)
         */
        switch (accessor.componentType) {
        case TINYGLTF_COMPONENT_TYPE_BYTE: {
            COPY_INDICES(int8_t)
        }
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE: {
            COPY_INDICES(uint8_t)
        }
        case TINYGLTF_COMPONENT_TYPE_SHORT: {
            COPY_INDICES(int16_t)
        }
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: {
            COPY_INDICES(uint16_t)
        }
        case TINYGLTF_COMPONENT_TYPE_INT: {
            COPY_INDICES(int32_t)
        }
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT: {
            COPY_INDICES(uint32_t)
        }
        default:
            break;
        }

        return { start, elements };
    }

    float4x4 getTransformMatrix(const gltf::Node& node) {
        const auto& matrix = node.matrix;
        if (matrix.size() == 16) {
            return float4x4::from(
                float4::from(float(matrix[0]), float(matrix[1]), float(matrix[2]), float(matrix[3])),
                float4::from(float(matrix[4]), float(matrix[5]), float(matrix[6]), float(matrix[7])),
                float4::from(float(matrix[8]), float(matrix[9]), float(matrix[10]), float(matrix[11])),
                float4::from(float(matrix[12]), float(matrix[13]), float(matrix[14]), float(matrix[15]))
            );
        } else {
            auto result = float4x4::scaling(1.f, 1.f, 1.f);

            const auto& scaling = node.scale;
            const auto& move = node.translation;
            const auto& rotation = node.rotation;
            
            if (move.size() == 3) {
                result *= float4x4::translation(float(move[0]), float(move[2]), float(move[1]));
            }

            if (rotation.size() == 4) {
                result *= float4x4::rotation(float(rotation[3]), float3::from(float(rotation[0]), float(rotation[1]), float(rotation[2])));
            }

            if (scaling.size() == 3) {
                result *= float4x4::scaling(float(scaling[0]), float(scaling[1]), float(scaling[2]));
            }

            return result;
        }
    }

    void buildNode(assets::World* world, const gltf::Model& model, DataSink* sink, float4x4 matrix, size_t index) {
        const auto& node = model.nodes[index];
        int meshIndex = node.mesh;

        auto transform = matrix * getTransformMatrix(node);

        if (meshIndex != -1) {
            const auto& mesh = model.meshes[node.mesh];
            const auto& primitives = mesh.primitives;

            for (const auto& primitive : primitives) {
                const auto& attributes = primitive.attributes;

                const auto getData = [&](const char* at) -> AttributeData {
                    const auto index = attributes.at(at);
                    if (index == -1) { return { nullptr, 0, 0 }; }

                    const auto& accessor = model.accessors[index];
                    const auto& bufferView = model.bufferViews[accessor.bufferView];
                    const auto& buffer = model.buffers[bufferView.buffer];

                    const auto data = buffer.data.data() + accessor.byteOffset + bufferView.byteOffset;
                    const auto length = accessor.count;
                    const auto stride = accessor.ByteStride(bufferView);

                    return { data, length, stride };
                };

                const auto positionData = getData("POSITION");
                const auto texData = getData("TEXCOORD_0");

                const auto getMaterial = [&]() -> assets::Material {
                    if (primitive.material == -1) { return { 0, 0 }; }
                    const auto& material = model.materials[primitive.material];
                    const auto& base = material.pbrMetallicRoughness.baseColorTexture;
                    if (base.index == -1) { return { 0, 0 }; }
                    const auto& texture = model.textures[base.index];
                    return { size_t(texture.source + 1), size_t(texture.sampler + 1) };
                };

                const auto material = getMaterial();

                // figure out if we need indices
                const auto& indexData = primitive.indices;
                if (indexData != -1) {
                    auto data = getVec3Array(positionData, texData, sink, false);
                    auto view = getIndexArray(model, sink, indexData);

                    data.views[0] = view;
                    data.material = material;
                    data.transform = transform;
                    world->meshes.push_back(data);
                } else {
                    auto data = getVec3Array(positionData, texData, sink, true);
                    
                    data.material = material;
                    data.transform = transform;

                    world->meshes.push_back(data);
                }
            }
        }

        for (int child : node.children) {
            buildNode(world, model, sink, transform, child);
        }
    }

    assets::Sampler::Wrap getWrap(int type) {
        switch (type) {
        case TINYGLTF_TEXTURE_WRAP_REPEAT:
            return assets::Sampler::Wrap::REPEAT;
        case TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE:
            return assets::Sampler::Wrap::CLAMP;
        case TINYGLTF_TEXTURE_WRAP_MIRRORED_REPEAT:
            return assets::Sampler::Wrap::MIRROR;
        default:
            return assets::Sampler::Wrap::CLAMP;
        }
    }

    assets::World* gltfWorld(std::string_view path) {
        log::loader->info("loading gltf file {}", path);
        std::string errors;
        std::string warnings;
        gltf::Model model;
        bool result = loader.LoadASCIIFromFile(&model, &errors, &warnings, path.data());

        if (!errors.empty()) {
            log::loader->fatal("{}", errors);
        }

        if (!warnings.empty()) {
            log::loader->warn("{}", warnings);
        }

        if (!result) {
            log::loader->fatal("failed to load gltf file: {}", path);
            return { };
        }

        auto* world = new assets::World();

        size_t numImages = model.images.size();
        size_t numSamplers = model.samplers.size();

        world->textures.resize(numImages + 1);
        world->textures[0] = assets::genMissingTexture({ 512, 512 }, assets::Texture::Format::RGB);

        world->samplers.resize(numSamplers + 1);
        world->samplers[0] = assets::genMissingSampler();

        for (size_t i = 0; i < numImages; i++) {
            const auto& image = model.images[i];
            assets::Texture texture = { 
                .name = image.name,
                .resolution = { size_t(image.width), size_t(image.height) },
                .depth = image.component == 3 ? assets::Texture::Format::RGB : assets::Texture::Format::RGBA,
                .data = std::move(image.image)
            };

            world->textures[i + 1] = texture;
        }

        for (size_t i = 0; i < numSamplers; i++) {
            const auto& sampler = model.samplers[i];
            world->samplers[i + 1] = { getWrap(sampler.wrapS), getWrap(sampler.wrapT) };
        }

        DataSink sink;
        const auto& scene = model.scenes[model.defaultScene];
        for (const auto& index : scene.nodes) {
            buildNode(world, model, &sink, float4x4::scaling(1.f, 1.f, 1.f), index);
        }

        world->indexBuffers.push_back(sink.indices);
        world->vertexBuffers.push_back(sink.vertices);

        newDebugObject(world);

        return world;
    }
}
