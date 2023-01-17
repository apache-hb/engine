#pragma once

#include "engine/render/resources/resources.h"

namespace simcoe::render {
    struct GlobalPass final : Pass {
        GlobalPass(const GraphObject& info);

        void init() override;
        void deinit() override;

        void imgui() override;

        void execute() override { }
    
        RenderTargetResource *rtvResource;
        Output *renderTargetOut;

    private:
        struct Link {
            int source;
            int destination;
        };

        std::unordered_map<Pass*, int> passIds;
        std::unordered_map<Wire*, int> wireIds;
        std::unordered_map<int, Link> links;
    };
}
