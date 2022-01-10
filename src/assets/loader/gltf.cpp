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
        return { vec->x, vec->y, vec->z };
    }

    struct MeshData {
        std::vector<assets::Vertex> vertices;
        std::vector<uint32_t> indices;
        assets::Mesh mesh;
    };

    MeshData getVec3Array(assets::World* world, const uint8_t* data, size_t elements, size_t stride, bool genIndices) {
        std::vector<assets::Vertex> vertices;
        std::vector<uint32_t> indices;
        std::unordered_map<float3, uint32_t> unique;

        for (size_t i = 0; i < elements; i++) {
            auto vec = getVec3(data + i * stride);

            if (genIndices) {
                if (auto iter = unique.find(vec); iter != unique.end()) {
                    indices.push_back(iter->second);
                } else {
                    auto index = uint32_t(vertices.size());

                    vertices.push_back({ vec, { 0.0f, 0.0f } });
                    indices.push_back(index);
                    unique[vec] = index;
                }
            } else {
                vertices.push_back({ vec, { 0.0f, 0.0f } });
            }
        }

        assets::IndexBufferView view = { .offset = 0, .length = indices.size() };

        assets::Mesh mesh = {
            .views = { view },
            .buffer = world->indexBuffers.size(),
            .texture = 0
        };

        return {
            .vertices = vertices,
            .indices = indices,
            .mesh = mesh
        };
    }

    std::vector<uint32_t> getIndexArray(const gltf::Model& model, int index) {
        const auto& accessor = model.accessors[index];
        const auto& bufferView = model.bufferViews[accessor.bufferView];
        const auto& buffer = model.buffers[bufferView.buffer];

        const auto data = buffer.data.data() + accessor.byteOffset + bufferView.byteOffset;
        const auto stride = accessor.ByteStride(bufferView);
        const auto elements = accessor.count;

        log::loader->info("loading {} elements of type {} with stride {}", elements, accessor.componentType, stride);

        std::vector<uint32_t> indices(elements);

#define COPY_INDICES(type) \
            const auto* begin = reinterpret_cast<const type*>(data); \
            const auto* end = reinterpret_cast<const type*>(data) + elements; \
            std::copy(begin, end, indices.begin()); \
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

        return indices;
    }

    void buildNode(assets::World* world, const gltf::Model& model, size_t index) {
        const auto& node = model.nodes[index];
        int meshIndex = node.mesh;

        if (meshIndex != -1) {
            const auto& mesh = model.meshes[node.mesh];
            const auto& primitives = mesh.primitives;

            for (const auto& primitive : primitives) {
                const auto& attributes = primitive.attributes;

                // get position data
                const auto& positionAccessor = model.accessors[attributes.at("POSITION")];
                const auto& positionBufferView = model.bufferViews[positionAccessor.bufferView];
                const auto& positionBuffer = model.buffers[positionBufferView.buffer];

                const auto positionData = positionBuffer.data.data() + positionBufferView.byteOffset + positionAccessor.byteOffset;
                const auto positionLength = positionAccessor.count;
                const auto positionStride = positionAccessor.ByteStride(positionBufferView);

                // figure out if we need indices
                const auto& indexData = primitive.indices;
                if (indexData != -1) {
                    auto data = getVec3Array(world, positionData, positionLength, positionStride, false);
                    auto indices = getIndexArray(model, indexData);

                    log::loader->info("indices {}", indices.size());

                    data.mesh.views[0].length = indices.size();

                    world->vertexBuffers.push_back(data.vertices);
                    world->indexBuffers.push_back(indices);
                    world->meshes.push_back(data.mesh);
                } else {
                    auto [vertices, indices, data] = getVec3Array(world, positionData, positionLength, positionStride, true);
                    world->vertexBuffers.push_back(vertices);
                    world->indexBuffers.push_back(indices);
                    world->meshes.push_back(data);
                }
            }
        }

        for (int child : node.children) {
            buildNode(world, model, child);
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

        world->textures.resize(numImages + 1);
        world->textures[0] = assets::genMissingTexture({ 512, 512 }, assets::Texture::Format::RGB);

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

        const auto& scene = model.scenes[model.defaultScene];
        for (const auto& index : scene.nodes) {
            buildNode(world, model, index);
        }

        newDebugObject(world);

        return world;
    }
}