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

GltfScene::GltfScene(Context* context, std::string_view path): Super(context) {
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
    createBuffers();
    createImages();
    Super::create();
}

void GltfScene::destroy() {
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
    for (const auto& image : model.images) {
        images.push_back(ctx->uploadTexture(strings::widen(image.uri), {
            .width = UINT(image.width),
            .height = UINT(image.height),
            .component = image.component,
            .data = { image.data }
        }));
    }
}

void GltfScene::destroyImages() {
    for (auto& image : images) {
        image.tryDrop();
    }
}

void GltfScene::drawNode(size_t index) {
    const auto& node = model.nodes[index];

    if (node.mesh >= 0) {
        const auto& mesh = model.meshes[node.mesh];

        for (auto& prim : mesh.primitives) {
            const auto& index = prim.indices;
        }
    }

    for (auto child : node.children) {
        drawNode(child);
    }
}

ID3D12CommandList* GltfScene::populate() {
    Super::begin();

    const auto& scene = model.scenes[model.defaultScene];
    for (auto index : scene.nodes) {
        drawNode(index);
    }

    Super::end();
    return commandList.get();
}

UINT GltfScene::requiredCbvSrvSize() const {
    return UINT(model.images.size());
}
