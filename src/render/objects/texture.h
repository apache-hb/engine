#pragma once

#include "resource.h"

namespace engine::render {
    struct TextureBuffer : Resource {
        struct Create {
            Resource texture;
            size_t mipLevels;
            DXGI_FORMAT format;
            Resolution resolution;
        };

        TextureBuffer() = default;
        TextureBuffer(Create create)
            : Resource(create.texture)
            , mipLevels(create.mipLevels)
            , format(create.format)
            , resolution(create.resolution)
        { }

        size_t mipLevels;
        DXGI_FORMAT format;
        Resolution resolution;
    };
}
