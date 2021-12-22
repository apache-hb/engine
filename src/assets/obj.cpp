#include "loader.h"

#include "tinyobj.h"

#include "logging/log.h"

#include <filesystem>
#include <unordered_map>

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

        Scene scene;

        log::loader->info("loading scene {}", path);
        log::loader->info("\t{} materials", materials.size());
        log::loader->info("\t{} shapes", shapes.size());
        log::loader->info("\t{} vertices", attrib.vertices.size() / 3);

        for (const auto& mat : materials) {
            auto texname = base/mat.diffuse_texname;
            auto tex = image(texname.string());
            if (tex.pixels.size() == 0) {
                scene.textures.push_back(generateTexture(512, 512));
            } else {
                scene.textures.push_back(tex);
            }
        }

        for (const auto& shape : shapes) {
            const auto& mesh = shape.mesh;

            size_t firstIndex = scene.indices.size();

            size_t skip = 0;
            size_t face = 0;
            size_t material = 0;

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
                
                if (skip > 0) {
                    skip -= 1;
                } else {
                    skip = mesh.num_face_vertices[face];
                    material = mesh.material_ids[face];

                    face += 1;
                }

                Vertex vertexData = { vertex, normal, texcoord, UINT(material) };

                scene.vertices.push_back(vertexData);
                scene.indices.push_back(UINT(scene.vertices.size() - 1));
            }

            size_t lastIndex = scene.indices.size() - 1;

            size_t totalIndices = lastIndex - firstIndex + 1;

            scene.objects.push_back({ firstIndex, totalIndices });
        }

        return scene;
    }
}
