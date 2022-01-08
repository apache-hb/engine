#include "assets/loader.h"
#include "logging/log.h"
#include "gltf/tinygltf.h"

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
        return { vec->x, vec->z, vec->y };
    }

    void getVec3Array(assets::World* world, const uint8_t* data, size_t elements, size_t stride) {
        std::vector<assets::Vertex> vertices;
        std::vector<uint32_t> indices;
        std::unordered_map<float3, uint32_t> unique;

        for (size_t i = 0; i < elements; i++) {
            auto vec = getVec3(data + i * stride);

            if (auto iter = unique.find(vec); iter != unique.end()) {
                indices.push_back(iter->second);
            } else {
                auto index = uint32_t(vertices.size());

                vertices.push_back({ vec, { 0.0f, 0.0f } });
                indices.push_back(index);
                unique[vec] = index;
            }
        }

        assets::IndexBufferView view = { .offset = 0, .length = indices.size() };

        assets::Mesh mesh = { 
            .views = { view },
            .buffer = world->indexBuffers.size(),
            .texture = 0
        };

        world->vertexBuffers.push_back(vertices);
        world->indexBuffers.push_back(indices);
        world->meshes.push_back(mesh);
    }

    void buildNode(assets::World* world, const gltf::Model& model, size_t index) {
        const auto& node = model.nodes[index];
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

            getVec3Array(world, positionData, positionLength, positionStride);
        }
    }

    assets::World gltfWorld(std::string_view path) {
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

        assets::World world;

        size_t numImages = model.images.size();

        world.textures.resize(numImages + 1);
        world.textures[0] = assets::genMissingTexture({ 512, 512 }, assets::Texture::Format::RGB);

        for (size_t i = 0; i < numImages; i++) {
            const auto& image = model.images[i];
            assets::Texture texture = { 
                .name = image.name,
                .resolution = { size_t(image.width), size_t(image.height) },
                .depth = image.component == 3 ? assets::Texture::Format::RGB : assets::Texture::Format::RGBA,
                .data = std::move(image.image)
            };

            world.textures[i + 1] = texture;
        }

        return world;
    }
}
