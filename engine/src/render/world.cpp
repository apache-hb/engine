#include "engine/render/world.h"

#include "engine/base/logging.h"
#include "engine/base/util.h"

#include <algorithm>
#include <unordered_map>

#include "tinygltf/tinygltf.h"

namespace gltf = tinygltf;

using namespace engine;

namespace {
    math::float3 loadVec3(std::span<const double> data) {
        return { .x = float(data[0]), .y = float(data[1]), .z = float(data[2]) };
    }

    math::float4 loadVec4(std::span<const double> data) {
        return { .x = float(data[0]), .y = float(data[1]), .z = float(data[2]), .w = float(data[3]) };
    }
    
    math::float4x4 createTransform(const gltf::Node& node) {
        if (node.matrix.size() == 16) {
            auto &mat = node.matrix;
            return math::float4x4::from(
                { float(mat[0]), float(mat[1]), float(mat[2]), float(mat[3]) },
                { float(mat[4]), float(mat[5]), float(mat[6]), float(mat[7]) },
                { float(mat[8]), float(mat[9]), float(mat[10]), float(mat[11]) },
                { float(mat[12]), float(mat[13]), float(mat[14]), float(mat[15]) }
            );
        }

        auto result = math::float4x4::identity();

        if (node.translation.size() == 3) {
            auto [x, y, z] = loadVec3(node.translation);
            result *= math::float4x4::translation(x, y, z);
        }

        if (node.rotation.size() == 4) {
            auto it = loadVec4(node.rotation);
            result *= math::float4x4::rotation(it.w, it.vec3());
        }

        if (node.scale.size() == 3) {
            auto [x, y, z] = loadVec3(node.scale);
            result *= math::float4x4::scaling(x, y, z);
        }

        return result;
    }

    std::vector<size_t> loadChildren(std::span<const int> data) {
        std::vector<size_t> result;
        std::copy(data.begin(), data.end(), std::back_inserter(result));
        return result;
    }

    size_t getTexture(const gltf::Model& model, const gltf::Primitive& primitive, const gltf::Mesh& mesh) {
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
    };

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
            .stride = size_t(stride) 
        };
    }

    struct Sink {
        Sink(const gltf::Model& model)
            : model(model)
        { }

        std::vector<size_t> getPrimitives(int index) {
            if (index == -1) { return { }; }
            const auto& mesh = model.meshes[index];

            for (const auto& primitive : mesh.primitives) {
                auto texture = getTexture(model, primitive, mesh);
                auto position = getAttribute(primitive.attributes.at("POSITION"), model);
                auto uv = getAttribute(primitive.attributes.at("TEXCOORD_0"), model);
                
            }
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
        channel.warn("warnings: {}", warn);
    }

    if (!err.empty()) {
        channel.fatal("errors: {}", err);
    }

    if (!result) {
        return { };
    }

    Sink sink{model};

    for (const auto& image : model.images) {
        Texture it = {
            .size = { size_t(image.width), size_t(image.height) }
        };

        it.data.resize(image.image.size());
        memcpy(it.data.data(), image.image.data(), it.data.size());

        sink.world.textures.push_back(it);
    }

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
