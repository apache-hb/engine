#pragma once

#include "engine/render/objects/view.h"

#include "engine/render/util.h"

namespace engine::render::d3d12 {
    template<typename T>
    concept IsGraphicsCommandList = std::is_base_of_v<ID3D12GraphicsCommandList, T>;

    template<IsGraphicsCommandList T = ID3D12GraphicsCommandList>
    struct GraphicsCommandList : Object<T> {
        using Super = Object<T>;
        using Super::Super;

        void setView(const d3d12::View &view) {
            Super::get()->RSSetViewports(1, &view.viewport);
            Super::get()->RSSetScissorRects(1, &view.scissor);
        }
    };
}
