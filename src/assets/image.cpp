#include "loader.h"

#include "stb_image.h"

#include "logging/log.h"

namespace engine::loader {
    Texture image(std::string_view path) {
        int width, height, channels;
        UINT8 *data = stbi_load(path.data(), &width, &height, &channels, 0);
        if (data == nullptr) { return {}; }

        size_t size = width * height;
        std::vector<UINT8> vec;
        
        if (channels == 3) {
            vec.reserve(size * 4);
            for (size_t i = 0; i < size; i++) {
                auto idx = i * 3;
                vec.push_back(data[idx + 0]);
                vec.push_back(data[idx + 1]);
                vec.push_back(data[idx + 2]);
                vec.push_back(0xFF);
            }
        } else {
            vec = std::vector<UINT8>(data, data + (size * channels));
        }

        log::global->info("loaded image {}: {}x{}({})", path, width, height, channels);

        Texture tex = {
            .width = size_t(width),
            .height = size_t(height),
            .bpp = 4,
            .pixels = vec
        };

        return tex;
    }
}
