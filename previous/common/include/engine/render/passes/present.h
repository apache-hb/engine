#pragma once

#include "engine/render/graph.h"

namespace simcoe::render {
    struct PresentPass final : Pass {
        PresentPass(const GraphObject& info);

        void execute() override;

        Input *renderTargetIn;
        Input *sceneTargetIn;
    };
}
