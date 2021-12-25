#include "scene.h"

namespace engine::assets {
    Scene::Scene(render::Context* ctx, std::string_view path): context(ctx), name(path) {
        model = gltfScene(path);

        loadImages();
    }

    void Scene::loadImages() {
        size_t numImages = model.images.size();
        images.reserve(numImages);

        for (auto& image : model.images) {
            const auto name = std::format("{}-{}", name, image.uri);
            auto it = context->uploadTexture(strings::widen(name), {
                .width = size_t(image.width),
                .height = size_t(image.height),
                .component = size_t(image.component),
                .data = image.data
            });

            images.emplace_back(it);
        }
    }
}
