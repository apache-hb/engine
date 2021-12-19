#include "loader.h"

#include "stb_image.h"

#include "logging/log.h"

namespace engine::loader {
    Texture image(std::string_view path) {
        int width, height, channels;
        UINT8 *data = stbi_load(path.data(), &width, &height, &channels, 0);
        if (data == nullptr) { return {}; }

        size_t size = width * height * channels;
        std::vector<UINT8> vec(data, data + size);

        log::global->info("loaded image {}: {}x{}", path, width, height);

        Texture tex = {
            .width = size_t(width),
            .height = size_t(height),
            .bpp = size_t(channels),
            .pixels = vec
        };

        return tex;
    }
}
