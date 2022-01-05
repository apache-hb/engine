#include "texture.h"

using namespace engine::assets;

template<bool Alpha>
constexpr std::vector<uint8_t> genData(const Texture::Resolution& resolution) {
    constexpr auto depth = Alpha ? 4 : 3;
    std::vector<uint8_t> data;

    data.resize(resolution.width * resolution.height * depth);

    for (size_t i = 0; i < data.size(); i += depth) {
        data[i + 0] = 0xFF;
        data[i + 1] = 0x00;
        data[i + 2] = 0xFF;
        
        if constexpr (Alpha) {
            data[i + 3] = 0xFF;
        }
    }

    return data;
}

Texture engine::assets::genMissingTexture(const Texture::Resolution& resolution, Texture::Format depth) {
    auto data = depth == Texture::Format::RGBA 
        ? genData<true>(resolution) 
        : genData<false>(resolution);

    return { "missing-texture", resolution, depth, data };
}
