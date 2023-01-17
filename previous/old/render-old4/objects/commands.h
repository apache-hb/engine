#pragma once

#include "render/util.h"

namespace engine::render {
    namespace commands {
        constexpr auto kDirect = D3D12_COMMAND_LIST_TYPE_DIRECT;
        constexpr auto kCopy = D3D12_COMMAND_LIST_TYPE_COPY;
        constexpr auto kBundle = D3D12_COMMAND_LIST_TYPE_BUNDLE;
    }
}
