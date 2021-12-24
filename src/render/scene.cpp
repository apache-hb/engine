#include "render.h"

namespace engine::render {
    void Context::createBuffers() {
        for (auto& buffer : gltf.buffers) {
            auto data = uploadSpan<unsigned char>(buffer.data);

            buffers.push_back(data);
        }
    }

    void Context::createImages() {
        for (auto& image : gltf.images) {
            auto texture = uploadTexture({
                .width = size_t(image.width),
                .height = size_t(image.height),
                .component = size_t(image.component),
                .data = image.image
            });

            textures.push_back(texture);
        }
    }

    void Context::createMeshes() {
        for (auto& mesh : gltf.meshes) {
            log::render->info("mesh: {}", mesh.name);
            for (auto& primitive : mesh.primitives) {
                log::render->info("\tprimitive: {}", primitive.mode);
            }
        }
    }
}
