#pragma once

#include "render/util.h"

namespace engine::render {
    struct Texture : Com<ID3D12Resource> {
        using Super = Com<ID3D12Resource>;

        struct Create {
            Super texture;
            size_t mipLevels;
            DXGI_FORMAT format;
            Resolution resolution;
        };

        Texture() = default;
        Texture(Create create)
            : Super(create.texture)
            , mipLevels(create.mipLevels)
            , format(create.format)
            , resolution(create.resolution)
        { }

        size_t mipLevels;
        DXGI_FORMAT format;
        Resolution resolution;
    };
}
