#include "engine/render/world.h"

#include "engine/base/logging.h"
#include "engine/base/util.h"

#include <algorithm>

#include "tinygltf/tinygltf.h"

namespace gltf = tinygltf;

using namespace engine;

render::World render::loadGltf(const char *path) {
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

    World world;

    for (const auto& image : model.images) {
        Texture texture = {
            .name = image.name,
            .size = { size_t(image.width), size_t(image.height) }
        };

        texture.data.resize(image.image.size());
        memcpy(texture.data.data(), image.image.data(), texture.data.size());

        world.textures.push_back(texture);
    }

    channel.info("finished loading gltf `{}`", path);

    return world;
}
