#include <fastgltf/fastgltf_parser.hpp>
#include <stb_image.h>

#include "fastgltf/fastgltf_types.hpp"
#include "simcoe/assets/assets.h"

#include "simcoe/core/util.h"
#include "simcoe/core/io.h"

#include "simcoe/simcoe.h"

#include <unordered_map>

using namespace simcoe;
using namespace simcoe::assets;
using namespace simcoe::math;

template<typename... T>
struct overloaded : T... { using T::operator()...; };

template<typename... T>
overloaded(T...) -> overloaded<T...>;

template<typename T>
struct std::hash<Vec3<T>> {
    constexpr std::size_t operator()(const Vec3<T>& v) const noexcept {
        return std::hash<T>{}(v.x) ^ std::hash<T>{}(v.y) ^ std::hash<T>{}(v.z);
    }
};

template<typename T>
struct std::hash<Vec2<T>> {
    constexpr std::size_t operator()(const Vec2<T>& v) const noexcept {
        return std::hash<T>{}(v.x) ^ std::hash<T>{}(v.y);
    }
};

template<>
struct std::hash<assets::Vertex> {
    constexpr std::size_t operator()(const assets::Vertex& v) const noexcept {
        return std::hash<float3>{}(v.position) ^ std::hash<float2>{}(v.uv);
    }
};

template<>
struct std::equal_to<assets::Vertex> {
    constexpr bool operator()(const assets::Vertex& lhs, const assets::Vertex& rhs) const noexcept {
        return lhs.position == rhs.position && lhs.uv == rhs.uv;
    }
};
namespace {
    constexpr fastgltf::Options kOptions = fastgltf::Options::LoadGLBBuffers | fastgltf::Options::LoadExternalBuffers;
    
    constexpr const char *gltfErrorToString(fastgltf::Error err) {
#define ERROR_CASE(x) case fastgltf::Error::x: return #x
        switch (err) {
        ERROR_CASE(None);
        ERROR_CASE(InvalidPath);
        ERROR_CASE(MissingExtensions);
        ERROR_CASE(UnknownRequiredExtension);
        ERROR_CASE(InvalidJson);
        ERROR_CASE(InvalidGltf);
        case fastgltf::Error::InvalidOrMissingAssetField: 
            return "InvalidOrMissingAssetField/InvalidGLB";
        ERROR_CASE(MissingField);
        ERROR_CASE(MissingExternalBuffer);
        ERROR_CASE(UnsupportedVersion);
        default: return "Unknown";
        }
    }

    constexpr float3 zup(const float *pData) {
        auto [x, y, z] = float3::from(pData);
        return float3::from(x, z, y); // gltf is y up, we are z up
    }

    using BufferData = std::span<const uint8_t>;
    using IndexBuffer = std::vector<uint32_t>;
    using VertexBuffer = std::vector<Vertex>;

    constexpr BufferData getBufferData(const fastgltf::DataSource& source, std::string_view name) {
        return std::visit(overloaded {
            [&](const fastgltf::sources::Vector& vector) -> BufferData {
                return BufferData(vector.bytes);
            },
            [&](auto&) -> BufferData {
                gAssetLog.warn("unknown buffer type ({})", name);
                return BufferData();
            }
        }, source);
    }

    struct AttributeData {
        BufferData data;
        size_t stride;
    };

    struct PrimitiveData {
        size_t texture;
        size_t vertices;
        size_t indices;
    };

    struct GltfUpload final : IUpload {
        GltfUpload(IScene& scene)
            : scene(scene)
        { }

        void detach(const fs::path& path) {
            thread = std::jthread([this, path] {
                fastgltf::Parser parser;
                fastgltf::GltfDataBuffer buffer;
                std::unique_ptr<fastgltf::glTF> data;

                buffer.loadFromFile(path);

                switch (fastgltf::determineGltfFileType(&buffer)) {
                case fastgltf::GltfType::glTF:
                    gAssetLog.info("Loading glTF file: {}", path.string());
                    data = parser.loadGLTF(&buffer, path.parent_path(), kOptions);
                    break;
                case fastgltf::GltfType::GLB:
                    gAssetLog.info("Loading GLB file: {}", path.string());
                    data = parser.loadBinaryGLTF(&buffer, path.parent_path(), kOptions);
                    break;

                default:
                    gAssetLog.warn("Unknown glTF file type");
                    return;
                }

                if (fastgltf::Error err = parser.getError(); err != fastgltf::Error::None) {
                    gAssetLog.warn("Failed to load glTF file: {} ({})", gltfErrorToString(err), fastgltf::to_underlying(err));
                    return;
                }

                if (fastgltf::Error err = data->parse(); err != fastgltf::Error::None) {
                    gAssetLog.warn("Failed to parse glTF file: {} ({})", gltfErrorToString(err), fastgltf::to_underlying(err));
                    return;
                }

                asset = data->getParsedAsset();

                load();
            });
        }

        void load() {
            const auto& images = asset->images;

            for (size_t i : util::progress(texProgress, images.size())) {
                loadTexture(i, images[i]);
            }

            const auto& meshes = asset->meshes;

            for (size_t i : util::progress(meshProgress, meshes.size())) {
                loadMesh(i, meshes[i]);
            }

            const auto& nodes = asset->nodes;

            for (size_t i : util::progress(nodeProgress, nodes.size())) {
                loadNode(i, nodes[i]);
            }

            for (size_t i = 0; i < nodes.size(); i++) {
                const auto& node = nodes[i];
                std::vector<size_t> children;
                for (const auto& child : node.children) {
                    children.push_back(nodeMap[child]);
                }

                scene.setNodeChildren(nodeMap[i], children);
            }
        }

    private:
        void loadTextures() {
            const auto& images = asset->images;

            for (size_t i : util::progress(texProgress, images.size())) {
                loadTexture(i, images[i]);
            }
        }

        void loadTexture(size_t i, const fastgltf::Image& image) {
            BufferData buffer = getBufferData(image.data, image.name);
            if (buffer.empty()) { return; }

            int width, height, channels;
            stbi_uc *pImage = stbi_load_from_memory(buffer.data(), static_cast<int>(buffer.size_bytes()), &width, &height, &channels, 4);
            if (pImage == nullptr) {
                gAssetLog.warn("Failed to load image ({})", image.name);
                textureMap[i] = scene.getDefaultTexture();
                return;
            }

            textureMap[i] = scene.addTexture({ pImage, size2::from(width, height) });
        }

        void loadNode(size_t index, const fastgltf::Node& node) {
            auto getTransform = [](const auto& transform) {
                return std::visit(overloaded {
                    [](const fastgltf::Node::TransformMatrix& matrix) {
                        return float4x4::from(matrix.data());
                    },
                    [](const fastgltf::Node::TRS& trs) {
                        auto result = float4x4::identity();

                        result *= float4x4::translation(trs.translation[0], trs.translation[1], trs.translation[2]);
                        result *= float4x4::rotation(trs.rotation[3], float3::from(trs.rotation[0], trs.rotation[1], trs.rotation[2]));
                        result *= float4x4::scaling(trs.scale[0], trs.scale[1], trs.scale[2]);

                        return result;
                    }
                }, transform);
            };

            auto transform = getTransform(node.transform);

            auto primitives = node.meshIndex.has_value()
                ? primitiveMap[node.meshIndex.value()]
                : std::vector<size_t>();

            nodeMap[index] = scene.addNode({ 
                .transform = transform, 
                .primitives = primitives
            });
        }

        PrimitiveData loadPrimitive(const fastgltf::Primitive& primitive, std::string_view name) {
            auto getAttribute = [&](const fastgltf::Primitive& primitive, const std::string& name) {
                const auto& attribute = primitive.attributes.find(name);
                if (attribute == primitive.attributes.end()) { return AttributeData(); }

                const auto& accessor = asset->accessors[attribute->second];
                const auto& bufferView = asset->bufferViews[accessor.bufferViewIndex.value()];
                const auto& buffer = asset->buffers[bufferView.bufferIndex];

                BufferData bufferData = getBufferData(buffer.data, buffer.name);
                if (bufferData.empty()) { return AttributeData(); }

                size_t stride = bufferView.byteStride.has_value() 
                    ?  bufferView.byteStride.value()
                    : fastgltf::getElementByteSize(accessor.type, accessor.componentType);

                AttributeData result = {
                    .data = BufferData(bufferData.data() + bufferView.byteOffset + accessor.byteOffset, accessor.count * stride),
                    .stride = stride
                };

                return result;
            };

            auto getIndexBuffer = [&](const fastgltf::Primitive& primitive) {
                if (!primitive.indicesAccessor.has_value()) { return IndexBuffer(); }

                const auto& accessor = asset->accessors[primitive.indicesAccessor.value()];
                const auto& bufferView = asset->bufferViews[accessor.bufferViewIndex.value()];
                const auto& buffer = asset->buffers[bufferView.bufferIndex];

                BufferData bufferData = getBufferData(buffer.data, buffer.name);
                if (bufferData.empty()) { return IndexBuffer(); }

                size_t stride = fastgltf::getElementByteSize(accessor.type, accessor.componentType);
                uint32_t mask = 0xFFFFFFFF >> (32 - (stride * 8));

                IndexBuffer result(accessor.count);
                for (size_t i = 0; i < accessor.count; i++) {
                    auto offset = accessor.byteOffset + bufferView.byteOffset + i * stride;
                    result[i] = *reinterpret_cast<const uint32_t*>(bufferData.data() + offset) & mask;
                }

                return result;
            };

            auto getTexture = [&](const fastgltf::Primitive& primitive) -> size_t {
                if (!primitive.materialIndex.has_value()) { return scene.getDefaultTexture(); }

                const auto& material = asset->materials[primitive.materialIndex.value()];
                const auto& pbrData = material.pbrData;
                if (!pbrData.has_value()) { return scene.getDefaultTexture(); }

                const auto& baseColor = pbrData->metallicRoughnessTexture;
                if (!baseColor.has_value()) { return scene.getDefaultTexture(); }

                const auto& texture = asset->textures[baseColor->textureIndex];
                if (!texture.imageIndex.has_value()) { return scene.getDefaultTexture(); }

                return textureMap[texture.imageIndex.value()];
            };

            const auto [vertexData, vertexStride] = getAttribute(primitive, "POSITION");
            const auto [uvData, uvStride] = getAttribute(primitive, "TEXCOORD_0");

            if (vertexData.empty()) {
                gAssetLog.warn("primitive (mesh=`{}`) has no vertex data", name);
                return PrimitiveData();
            }

            if (uvData.empty()) {
                gAssetLog.warn("primitive (mesh=`{}`) has no uv data", name);
                return PrimitiveData();
            }

            std::unordered_map<Vertex, uint32_t> indexCache;

            std::vector<Vertex> vertices;
            std::vector<uint32_t> indices = getIndexBuffer(primitive);

            bool bHasIndices = !indices.empty();
            size_t vertexCount = bHasIndices ? indices.size() : (vertexData.size() / vertexStride);

            for (size_t i = 0; i < vertexCount; i++) {
                float3 position = zup(reinterpret_cast<const float*>(vertexData.data() + i * vertexStride));
                float2 uv = float2::from(reinterpret_cast<const float*>(uvData.data() + i * uvStride));
            
                Vertex vertex = { .position = position, .uv = uv };

                if (bHasIndices) {
                    vertices.push_back(vertex);
                } else {
                    if (indexCache.find(vertex) == indexCache.end()) {
                        indexCache[vertex] = static_cast<uint32_t>(vertices.size());
                        vertices.push_back(vertex);
                    }

                    indices.push_back(indexCache[vertex]);
                }
            }

            size_t vertexBufferIndex = scene.addVertexBuffer(vertices);
            size_t indexBufferIndex = scene.addIndexBuffer(indices);
            size_t texture = getTexture(primitive);

            return PrimitiveData {
                .texture = texture,
                .vertices = vertexBufferIndex,
                .indices = indexBufferIndex
            };
        }

        void loadMesh(size_t index, const fastgltf::Mesh& mesh) {
            for (const auto& primitive : mesh.primitives) {
                auto [texture, vertices, indices] = loadPrimitive(primitive, mesh.name);
                if (texture == SIZE_MAX || vertices == SIZE_MAX || indices == SIZE_MAX) { continue; }
            
                primitiveMap[index].push_back(scene.addPrimitive({
                    .vertexBuffer = vertices,
                    .indexBuffer = indices,
                    .texture = texture
                }));
            }
        }

        std::jthread thread;

        std::unordered_map<size_t, size_t> textureMap;
        std::unordered_map<size_t, size_t> nodeMap;
        std::unordered_map<size_t, std::vector<size_t>> primitiveMap;
        std::unordered_map<size_t, size_t> materialMap;

        std::unique_ptr<fastgltf::Asset> asset;
        IScene& scene;

    public:
        util::Progress<size_t> texProgress;
        util::Progress<size_t> nodeProgress;
        util::Progress<size_t> meshProgress;

        bool isDone() const override {
            return texProgress.done() && nodeProgress.done() && meshProgress.done();
        }

        float getProgress() const override {
            return (texProgress.fraction() + nodeProgress.fraction() + meshProgress.fraction()) / 3.0f;
        }
    };
}

std::shared_ptr<IUpload> Manager::gltf(const fs::path& path, IScene& scene) {
    auto result = std::make_shared<GltfUpload>(scene);
    result->detach(path);
    return result;
}

std::vector<std::byte> Manager::load(const fs::path& path) {
    std::unique_ptr<Io> file{Io::open((root / path).string(), Io::eRead)};
    return file->read<std::byte>();
}
