#pragma once

#include "engine/rhi/rhi.h"

namespace simcoe::graph {
    struct RenderGraph;

    struct Info {
        RenderGraph *parent;
        const char *name;
    };

    struct Create {
        Window *window;
        rhi::TextureSize resolution = { 1280, 720 }; // default resolution
        size_t backBuffers = 2;

        // tuning constants
        size_t heapSize = 1024; // size of the descriptor heap
        const char *root = "present"; // name of the root render pass
    };
}
