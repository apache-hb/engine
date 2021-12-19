#include "loader.h"

#include "tinyobj.h"

#include "logging/log.h"

#include <filesystem>
#include <unordered_map>

template<>
struct std::hash<engine::loader::Vertex> {
    size_t operator()(const engine::loader::Vertex &vertex) const {
        return std::hash<float>()(vertex.position.x) ^
            std::hash<float>()(vertex.position.y) ^
            std::hash<float>()(vertex.position.z) ^
            std::hash<float>()(vertex.normal.x) ^
            std::hash<float>()(vertex.normal.y) ^
            std::hash<float>()(vertex.normal.z) ^
            std::hash<float>()(vertex.texcoord.x) ^
            std::hash<float>()(vertex.texcoord.y);
    }
};

template<>
struct std::equal_to<engine::loader::Vertex> {
    bool operator()(const engine::loader::Vertex &lhs, const engine::loader::Vertex &rhs) const {
        return memcmp(&lhs, &rhs, sizeof(engine::loader::Vertex)) == 0;
    }
};

namespace {
    constexpr auto pixelSize = 4;

    engine::loader::Texture generateTexture(size_t width, size_t height) {
        const size_t size = width * height * pixelSize;

        std::vector<UINT8> texture(size);

        for (size_t n = 0; n < size; n += pixelSize) {
            texture[n + 0] = 0xFF; // red
            texture[n + 1] = 0x00; // green
            texture[n + 2] = 0xFB; // blue
            texture[n + 3] = 0xFF; // alpha
        }

        engine::loader::Texture tex = { width, height, pixelSize, std::move(texture) };

        return tex;
    }
}

namespace engine::loader {
    Scene objScene(std::string_view path) {
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn;
        std::string err;

        auto base = std::filesystem::path(path).parent_path();

        if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.data(), base.string().c_str())) {
            throw LoadError(err, warn);
        }

        // push default material
        materials.push_back(tinyobj::material_t {});

        Scene scene;

        for (const auto& mat : materials) {
            auto texname = base/mat.diffuse_texname;
            auto tex = image(texname.string());
            if (tex.pixels.size() == 0) {
                scene.textures.push_back(generateTexture(512, 512));
            } else {
                scene.textures.push_back(tex);
            }
        }

        std::unordered_map<Vertex, DWORD> uniqueVertices = {};

        for (const auto& shape : shapes) {
            const auto& mesh = shape.mesh;

            size_t firstIndex = scene.indices.size();

            for (const auto& index : mesh.indices) {
                auto vidx = index.vertex_index;
                auto nidx = index.normal_index;
                auto tidx = index.texcoord_index;

                XMFLOAT3 vertex = XMFLOAT3(
                    attrib.vertices[3 * vidx + 2],
                    attrib.vertices[3 * vidx + 1],
                    attrib.vertices[3 * vidx + 0]
                );

                XMFLOAT3 normal = XMFLOAT3(
                    attrib.normals[3 * nidx + 0],
                    attrib.normals[3 * nidx + 1],
                    attrib.normals[3 * nidx + 2]
                );

                XMFLOAT2 texcoord = XMFLOAT2(
                    attrib.texcoords[2 * tidx + 0],
                    attrib.texcoords[2 * tidx + 1]
                );
                
                Vertex vertexData = { vertex, normal, texcoord };

                if (auto it = uniqueVertices.find(vertexData); it != uniqueVertices.end()) {
                    scene.indices.push_back(it->second);
                } else {
                    scene.vertices.push_back(vertexData);
                    UINT indecie = UINT(scene.vertices.size() - 1);
                    scene.indices.push_back(indecie);
                    uniqueVertices[vertexData] = indecie;
                }
            }

            size_t lastIndex = scene.indices.size() - 1;

            size_t totalIndices = lastIndex - firstIndex + 1;

            scene.objects.push_back({ firstIndex, totalIndices, size_t(mesh.material_ids[0]) });
        }

        return scene;
    }
}
