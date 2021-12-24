#include "render.h"

namespace engine::render {
    void Context::createBuffers() {
        for (auto& buffer : gltf.buffers) {
            auto data = uploadSpan(buffer.data);

            buffers.push_back(data);
        }
    }

    void Context::createImages() {
        for (auto& image : gltf.images) {
            auto texture = uploadTexture({
                .width = image.width,
                .height = image.height,
                .component = image.component,
                .data = image.data
            });

            textures.push_back(texture);
        }
    }

    constexpr DXGI_FORMAT getFormat() {

    }

    void Context::createMeshes() {
        for (auto& mesh : gltf.meshes) {
            for (auto& primitive : mesh.primitives) {
                
            }
        }
    }
}
