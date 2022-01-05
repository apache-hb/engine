#pragma once

#include <string_view>
#include <vector>
#include "math/math.h"

namespace engine::assets {
    struct Texture {
        using Resolution = math::Resolution<size_t>;
        enum class Format {
            RGB,
            RGBA
        };

        std::string_view name;
        Resolution resolution;
        Format depth;
        std::vector<uint8_t> data;
    };

    Texture genMissingTexture(const Texture::Resolution& resolution, Texture::Format depth);
}
