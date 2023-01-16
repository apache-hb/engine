#pragma once

#include "engine/render/resources/resources.h"

namespace simcoe::render {
    using ImGuiUpdate = std::function<void()>;

    struct ImGuiPass final : Pass {
        ImGuiPass(const GraphObject& info, ImGuiUpdate update);

        void init() override;
        void deinit() override;
        void execute() override;

        WireHandle<Input, RenderTargetResource> renderTargetIn;
        Output *renderTargetOut;

    private:
        size_t imguiHeapOffset;
        ImGuiUpdate update;
    };
}
