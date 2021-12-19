#include "loader.h"

#include "stb_image.h"

#include "logging/log.h"

namespace engine::loader {
    Texture tga(std::string_view path) {
        int width, height, channels;
        UINT8 *data = stbi_load(path.data(), &width, &height, &channels, 0);

        log::global->info("loaded image {}: {}x{}x{} {}", path, width, height, channels, (void*)data);

        Texture tex = {
            .width = size_t(width), 
            .height = size_t(height), 
            .bpp = size_t(channels),
            .pixels = std::span<UINT8>(data, width * height * channels)
        };

        return tex;
    }
}
