#pragma once

#include "render/render.h"

namespace engine::render {
    struct Scene {
        Scene(Context* ctx);
        virtual ~Scene() = default;

    protected:
        void begin(Commands* commands);
        void end(Commands* commands);
        
        DescriptorHeap dsvHeap;
        Resource depthBuffer;
    };
}
