#include <fastgltf/fastgltf_parser.hpp>
#include <stb_image.h>

#include "simcoe/assets/assets.h"

#include "simcoe/core/util.h"

#include "simcoe/simcoe.h"

#include <unordered_map>

using namespace simcoe;
using namespace simcoe::assets;

template<typename... T>
struct overloaded : T... { using T::operator()...; };

template<typename... T>
overloaded(T...) -> overloaded<T...>;

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
        return std::hash<math::float3>{}(v.position) ^ std::hash<math::float2>{}(v.uv);
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

    constexpr math::float3 zup(const math::float3& v) {
        return math::float3::from(v.x, v.z, -v.y);
    }

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
        }

    private:
        void loadTextures() {
            const auto& images = asset->images;

            for (size_t i : util::progress(texProgress, images.size())) {
                loadTexture(i, images[i]);
            }
        }

        void loadTexture(size_t i, const fastgltf::Image& image) {
            std::visit(overloaded {
                [&](const fastgltf::sources::Vector& vector) {
                    const auto& [bytes, mime] = vector;
                    int width, height, channels;
                    
                    stbi_uc *pData = stbi_load_from_memory(bytes.data(), static_cast<int>(bytes.size()), &width, &height, &channels, 4);
                    if (pData == nullptr) {
                        gAssetLog.warn("Failed to load image ({})", image.name);
                        return;
                    }

                    textureMap[i] = scene.addTexture({ pData, math::size2::from(width, height) });
                
                    stbi_image_free(pData);
                },
                [&](auto&) {
                    gAssetLog.warn("unknown image source ({})", image.name);
                }
            }, image.data);
        }

        void loadNode(size_t index, const fastgltf::Node& node) {
            auto getTransform = [](const auto& transform) {
                return std::visit(overloaded {
                    [](const fastgltf::Node::TransformMatrix& matrix) {
                        return math::float4x4::from(matrix.data());
                    },
                    [](const fastgltf::Node::TRS& trs) {
                        auto result = math::float4x4::identity();

                        result *= math::float4x4::translation(trs.translation[0], trs.translation[1], trs.translation[2]);
                        result *= math::float4x4::rotation(trs.rotation[3], math::float3::from(trs.rotation[0], trs.rotation[1], trs.rotation[2]));
                        result *= math::float4x4::scaling(trs.scale[0], trs.scale[1], trs.scale[2]);

                        return result;
                    }
                }, transform);
            };

            auto transform = getTransform(node.transform);

            nodeMap[index] = scene.addNode({ 
                .transform = transform, 
                .children = {},
                .meshes = { meshMap[node.meshIndex.value()] }
            });
        }

        void loadMesh(size_t index, const fastgltf::Mesh&) {
            meshMap[index] = scene.addMesh({

            });
        }

        std::jthread thread;

        std::unordered_map<size_t, size_t> textureMap;
        std::unordered_map<size_t, size_t> nodeMap;
        std::unordered_map<size_t, size_t> meshMap;
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

std::shared_ptr<IUpload> assets::gltf(const fs::path& path, IScene& scene) {
    auto result = std::make_shared<GltfUpload>(scene);
    result->detach(path);
    return result;
}
