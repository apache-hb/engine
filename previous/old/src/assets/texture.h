#pragma once

#include <string>
#include <vector>
#include "math/math.h"

namespace engine::assets {
    struct Texture {
        using Resolution = math::Resolution<size_t>;
        enum class Format {
            RGB,
            RGBA
        };

        std::string name;
        Resolution resolution;
        Format depth;
        std::vector<uint8_t> data;
    };

    struct Sampler {
        enum Wrap { REPEAT, MIRROR, CLAMP };

        Wrap wrapU;
        Wrap wrapV;
    };

    Texture genMissingTexture(const Texture::Resolution& resolution, Texture::Format depth);
    Sampler genMissingSampler();
}
