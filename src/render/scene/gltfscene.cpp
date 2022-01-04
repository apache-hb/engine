#include "gltfscene.h"

#include "render/render.h"

#include "logging/log.h"

using namespace engine::render;

constexpr DXGI_FORMAT getAccessorFormat(int type) {
    switch (type) {
    case TINYGLTF_TYPE_VEC2: return DXGI_FORMAT_R32G32_FLOAT;
    case TINYGLTF_TYPE_VEC3: return DXGI_FORMAT_R32G32B32_FLOAT;
    case TINYGLTF_TYPE_VEC4: return DXGI_FORMAT_R32G32B32A32_FLOAT;
    default: return DXGI_FORMAT_UNKNOWN;
    }
}

constexpr DXGI_FORMAT getComponentFormat(int type) {
    switch (type) {
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE: return DXGI_FORMAT_R8_UINT;
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: return DXGI_FORMAT_R16_UINT;
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT: return DXGI_FORMAT_R32_UINT;
    default: return DXGI_FORMAT_UNKNOWN;
    }
}

constexpr D3D_PRIMITIVE_TOPOLOGY getPrimitiveTopology(int mode) {
    switch (mode) {
    case TINYGLTF_MODE_POINTS: return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
    case TINYGLTF_MODE_LINE: return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
    case TINYGLTF_MODE_LINE_STRIP: return D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;
    case TINYGLTF_MODE_TRIANGLES: return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    case TINYGLTF_MODE_TRIANGLE_STRIP: return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
    default: return D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
    }
}

GltfScene::GltfScene(Context* context, Camera* camera, std::string_view path): Super(context, camera) {
    std::string error;
    std::string warning;
    gltf::TinyGLTF loader;
    bool result = loader.LoadASCIIFromFile(&model, &error, &warning, path.data());

    if (!warning.empty()) {
        log::loader->warn("{}", warning);
    }

    if (!error.empty()) { 
        log::loader->fatal("{}", error);
    }

    if (!result) { throw engine::Error("failed to load gltf file"); }
}

void GltfScene::create() {
    Super::create();
    createBuffers();
    createImages();
    createMeshes();
}

void GltfScene::destroy() {
    destroyMeshes();
    destroyImages();
    destroyBuffers();
    Super::destroy();
}

void GltfScene::createBuffers() {
    for (const auto& buffer : model.buffers) {
        const auto& data = buffer.data;
        buffers.push_back(ctx->uploadData(strings::widen(buffer.uri), data.data(), data.size()));
    }
}

void GltfScene::destroyBuffers() {
    for (auto& buffer : buffers) {
        buffer.tryDrop();
    }
}

void GltfScene::createImages() {
    for (UINT i = 0; i < UINT(model.images.size()); i++) {
        const auto& image = model.images[i];
        auto texture = ctx->uploadTexture(strings::widen(image.uri), {
            .width = UINT(image.width),
            .height = UINT(image.height),
            .component = UINT(image.component),
            .data = { image.image }
        });
        getDevice()->CreateShaderResourceView(texture.get(), nullptr, cbvSrvCpuHandle(i));
        images.push_back(texture);
    }
}

void GltfScene::destroyImages() {
    for (auto& image : images) {
        image.tryDrop();
    }
}

void GltfScene::createMeshes() {
    for (const auto& mesh : model.meshes) {
        Mesh data;
        for (const auto& primitive : mesh.primitives) {
            Primitive prim;
            std::vector<Attribute> attributes;

            for (const auto& [name, index] : primitive.attributes) {
                const auto& accessor = model.accessors[index];
                const auto& bufferView = model.bufferViews[accessor.bufferView];

                const auto format = getAccessorFormat(accessor.type);
                const auto location = buffers[bufferView.buffer].gpuAddress() + bufferView.byteOffset + accessor.byteOffset;
                const auto size = UINT(bufferView.byteLength - accessor.byteOffset);
                const auto stride = UINT(accessor.ByteStride(bufferView));
            
                attributes.push_back({ format, { location, size, stride }});
            }

            const auto position = primitive.attributes.at("POSITION");
            const auto vertexCount = UINT(model.accessors[position].count);
            const auto topology = getPrimitiveTopology(primitive.mode);

            prim.attributes = attributes;
            prim.vertexCount = vertexCount;
            prim.topology = topology;

            if (primitive.indices >= 0) {
                const auto& accessor = model.accessors[primitive.indices];
                const auto& bufferView = model.bufferViews[accessor.bufferView];

                const auto format = getComponentFormat(accessor.componentType);
                const auto indexCount = UINT(accessor.count);
                const auto location = buffers[bufferView.buffer].gpuAddress() + bufferView.byteOffset + accessor.byteOffset;
                const auto size = UINT(bufferView.byteLength - accessor.byteOffset);

                prim.indexCount = indexCount;
                prim.indexView = { location, size, format };
            }

            const auto material = primitive.material;
            const auto it = model.materials[material];
            const auto it2 = it.pbrMetallicRoughness.baseColorTexture.index;
            
            prim.textureIndex = it2;
            data.primitives.push_back(prim);
        }
        meshes.push_back(data);
    }
}

void GltfScene::destroyMeshes() {
    meshes.clear();
}

void GltfScene::drawNode(size_t index) {
    const auto& node = model.nodes[index];

    if (node.mesh >= 0) {
        const auto& mesh = meshes[node.mesh];

        for (const auto& primitive : mesh.primitives) {
            commandList->IASetPrimitiveTopology(primitive.topology);
            commandList->SetGraphicsRoot32BitConstant(1, primitive.textureIndex, 0);

            for (auto i = 0; i != primitive.attributes.size(); i++) {
                const auto& attribute = primitive.attributes[i];
                commandList->IASetVertexBuffers(i, 1, &attribute.view);
            }

            if (primitive.indexCount > 0) {
                commandList->IASetIndexBuffer(&primitive.indexView);
                commandList->DrawIndexedInstanced(primitive.indexCount, 1, 0, 0, 0);
            } else {
                commandList->DrawInstanced(primitive.vertexCount, 1, 0, 0);
            }
        }
    }

    for (auto child : node.children) {
        drawNode(child);
    }
}

ID3D12CommandList* GltfScene::populate() {
    Super::begin();

    const auto& scene = model.scenes[model.defaultScene];
    for (auto node : scene.nodes) {
        drawNode(node);
    }

    Super::end();
    return commandList.get();
}

UINT GltfScene::requiredCbvSrvSize() const {
    return UINT(model.images.size());
}
