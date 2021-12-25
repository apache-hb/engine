#include "render.h"

namespace engine::render {
    std::vector<assets::Image> Context::loadImages(std::string_view path, const assets::gltf::Model& gltf) {
        auto& gltfImages = gltf.images;
        size_t numImages = gltfImages.size();
        std::vector<assets::Image> images(numImages);

        for (auto& image : gltf.images) {
            const auto name = std::format(L"{}-{}", strings::widen(path), strings::widen(image.uri));
            auto resource = uploadTexture(name, {
                .width = size_t(image.width),
                .height = size_t(image.height),
                .component = size_t(image.component),
                .data = image.data
            });

            images.push_back({ resource });
        }

        return images;
    }

    assets::Mesh Context::loadMesh(std::string_view path, const assets::gltf::Model& gltf, const assets::gltf::Mesh& mesh) {
        auto& gltfPrimitives = mesh.primitives;
        size_t numPrimitives = gltfPrimitives.size();
        std::vector<assets::Primitive> primitives(numPrimitives);
        std::vector<assets::IndexedPrimitive> indexedPrimitives(numPrimitives);

        for (auto& primitive : gltfPrimitives) {
            auto getAttribute = [&](const std::string& name) {
                auto it = primitive.attributes.find(name);
                if (it == primitive.attributes.end()) { return -1; }
                return it->second;
            };

            auto position = getAttribute("POSITION");
            auto normal = getAttribute("NORMAL");
            auto tangent = getAttribute("TANGENT");
            auto texcoord = getAttribute("TEXCOORD_0");
            
            std::map<std::string_view, std::string_view> defines = {
                { "HAS_NORMAL", normal != -1 ? "1" : "0" },
                { "HAS_TANGENT", tangent != -1 ? "1" : "0" },
                { "HAS_TEXCOORD", texcoord != -1 ? "1" : "0" },
            };

            std::vector<ShaderInput> input = { shaderInput("POSITION", DXGI_FORMAT_R32G32B32_FLOAT) };

            if (normal != -1) { input.push_back(shaderInput("NORMAL", DXGI_FORMAT_R32G32B32_FLOAT)); }
            if (tangent != -1) { input.push_back(shaderInput("TANGENT", DXGI_FORMAT_R32G32B32A32_FLOAT)); }
            if (texcoord != -1) { input.push_back(shaderInput("TEXCOORD", DXGI_FORMAT_R32G32_FLOAT)); }

            ShaderLibrary::Create create = {
                .path = "resources\\shaders\\scene-shader.hlsl",
                .vsMain = "vsMain",
                .psMain = "psMain",
                .layout = input
            };

            auto shaders = ShaderLibrary(create, defines);

            if (primitive.indices >= 0) {
                const auto& accessor = gltf.accessors[primitive.indices];
                const auto& bufferView = gltf.bufferViews[accessor.bufferView];
                const auto& buffer = gltf.buffers[bufferView.buffer];
            }
        }

        return { primitives, indexedPrimitives };
    }

    std::vector<assets::Mesh> Context::loadMeshes(std::string_view path, const assets::gltf::Model& gltf) {
        auto& gltfMeshes = gltf.meshes;
        size_t numMeshes = gltfMeshes.size();
        std::vector<assets::Mesh> meshes(numMeshes);

        for (auto& mesh : gltf.meshes) {
            meshes.push_back(loadMesh(path, gltf, mesh));
        }

        return meshes;
    }

    assets::Scene Context::loadScene(std::string_view path) {
        auto gltf = assets::gltfScene(path);
        
        /// the total number of srv descriptors required
        auto srvCount = UINT(gltf.images.size() + SceneResources::Total);

        const auto name = std::format(L"{}-srv", strings::widen(path));
        auto srvHeap = device.newHeap(name, {
            .Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
            .NumDescriptors = srvCount,
            .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
        });

        auto images = loadImages(path, gltf);
        auto meshes = loadMeshes(path, gltf);

        return { srvHeap, images, meshes };
    }
}
