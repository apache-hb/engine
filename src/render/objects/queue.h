#pragma once

#include "commands.h"

namespace engine::render {
    struct CommandQueue : Object<ID3D12CommandQueue> {
        OBJECT(CommandQueue, ID3D12CommandQueue);
    };
}
