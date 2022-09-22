#pragma once

#include "dx/d3dx12.h"

namespace engine::render::d3d12 {
    struct View {
        View() : View(0, 0) { }

        View(int width, int height)
            : viewport(0.f, 0.f, float(width), float(height))
            , scissor(0, 0, width, height)
        { }

        CD3DX12_VIEWPORT viewport;
        CD3DX12_RECT scissor;
    };
}
