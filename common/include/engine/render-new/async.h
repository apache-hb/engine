#pragma once

#include "engine/rhi/rhi.h"

#include <memory>

namespace simcoe::render {
    struct IAsync {
        virtual ~IAsync() = default;

        virtual void apply(rhi::CommandList& list) = 0;

        virtual rhi::Buffer getBuffer() = 0;
    };

    using AsyncAction = std::shared_ptr<IAsync>;
}
