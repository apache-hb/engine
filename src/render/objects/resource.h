#pragma once

#include "render/util.h"

namespace engine::render {
    struct Resource : Object<ID3D12Resource> {
        OBJECT(Resource, ID3D12Resource);
    };
}
