#include "assets/loader.h"
#include "logging/log.h"
#include "gltf/tinygltf.h"
#include "debug.h"

#include <unordered_map>
#include <DirectXMath.h>

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
    using namespace DirectX;
    using namespace engine::math;
    namespace gltf = tinygltf;

    gltf::TinyGLTF loader;

    // gltf operates with +y up, we use +z up
    // so we need to flip the y and z axis
    XMVECTOR getVec3(const uint8_t* data) {
        const float3* vec = reinterpret_cast<const float3*>(data);
        return XMVectorSet(vec->x, vec->z, vec->y, 0.f);
    }

    float2 getVec2(const uint8_t* data) {
        const float2* vec = reinterpret_cast<const float2*>(data);
        return { vec->x, vec->y };
    }

    using TransformMatrix = XMMATRIX;

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

    MeshData getVec3Array(assets::World* world, const AttributeData& position, const AttributeData& texcoord, const TransformMatrix& transform, bool genIndices) {
        std::vector<assets::Vertex> vertices;
        std::vector<uint32_t> indices;
        std::unordered_map<float3, uint32_t> unique;

        for (size_t i = 0; i < position.length; i++) {
            auto xmvec = getVec3(position.data + i * position.stride);
            auto transformed = XMVector3Transform(xmvec, transform);
            auto vec = float3::from(XMVectorGetX(transformed), XMVectorGetY(transformed), XMVectorGetZ(transformed));
            auto coords = position.data == nullptr ? float2 { 0.f, 0.f } 
                : getVec2(texcoord.data + i * texcoord.stride);

            if (genIndices) {
                if (auto iter = unique.find(vec); iter != unique.end()) {
                    indices.push_back(iter->second);
                } else {
                    auto index = uint32_t(vertices.size());

                    vertices.push_back({ vec, coords });
                    indices.push_back(index);
                    unique[vec] = index;
                }
            } else {
                vertices.push_back({ vec, coords });
            }
        }

        assets::IndexBufferView view = { .offset = 0, .length = indices.size() };

        assets::Mesh mesh = {
            .views = { view },
            .buffer = world->indexBuffers.size(),
            .material = { 0, 0 }
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

    XMMATRIX getMatrix(const std::vector<double>& vec) {
        auto mat = XMFLOAT4X4(
            float(vec[0]), float(vec[1]), float(vec[2]), float(vec[3]),
            float(vec[4]), float(vec[5]), float(vec[6]), float(vec[7]),
            float(vec[8]), float(vec[9]), float(vec[10]), float(vec[11]),
            float(vec[12]), float(vec[13]), float(vec[14]), float(vec[15])
        );
        return XMLoadFloat4x4(&mat);
    }

    XMVECTOR getVector(const std::vector<double>& vec) {
        auto data = XMFLOAT4(float(vec[0]), float(vec[1]), float(vec[2]), float(vec[3]));
        return XMLoadFloat4(&data);
    }

    void buildNode(assets::World* world, const gltf::Model& model, const TransformMatrix& matrix, size_t index) {
        const auto& node = model.nodes[index];
        int meshIndex = node.mesh;

        auto transform = matrix;

        if (!node.matrix.empty()) {
            transform = XMMatrixMultiply(matrix, getMatrix(node.matrix));
        } else {
            if (!node.translation.empty()) {
                auto vec = getVector(node.translation);
                auto translate = XMVector3Transform(vec, matrix);
                transform = XMMatrixMultiply(XMMatrixTranslationFromVector(translate), matrix);
            }

            if (!node.rotation.empty()) {
                auto vec = getVector(node.rotation);
                auto rotate = XMQuaternionRotationRollPitchYawFromVector(vec);
                transform = XMMatrixMultiply(XMMatrixRotationQuaternion(rotate), transform);
            }

            if (!node.scale.empty()) {
                auto vec = getVector(node.scale);
                auto scale = XMVector3Transform(vec, matrix);
                transform = XMMatrixMultiply(XMMatrixScalingFromVector(scale), transform);
            }
        }

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
                    auto data = getVec3Array(world, positionData, texData, transform, false);
                    auto indices = getIndexArray(model, indexData);

                    data.mesh.views[0].length = indices.size();
                    data.mesh.material = material;

                    world->vertexBuffers.push_back(data.vertices);
                    world->indexBuffers.push_back(indices);
                    world->meshes.push_back(data.mesh);
                } else {
                    auto [vertices, indices, data] = getVec3Array(world, positionData, texData, transform, true);
                    
                    data.material = material;

                    world->vertexBuffers.push_back(vertices);
                    world->indexBuffers.push_back(indices);
                    world->meshes.push_back(data);
                }
            }
        }

        for (int child : node.children) {
            buildNode(world, model, transform, child);
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
        auto matrix = XMMatrixScaling(1.f, 1.f, 1.f);

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

        const auto& scene = model.scenes[model.defaultScene];
        for (const auto& index : scene.nodes) {
            buildNode(world, model, matrix, index);
        }

        newDebugObject(world);

        return world;
    }
}
