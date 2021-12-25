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
            auto it = context->uploadImage(strings::wident(name), {

            });
        }
    }
}
