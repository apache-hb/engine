#pragma once

#include "render/util.h"

namespace engine::render {
    struct RootSignature : Object<ID3D12RootSignature> {
        using Super = Object<ID3D12RootSignature>;

        RootSignature() = default;
        RootSignature(ID3D12Device* device, Com<ID3DBlob> blob);
    };

    namespace root {
        struct Create {
            std::span<const CD3DX12_ROOT_PARAMETER1> params;
            std::span<const D3D12_STATIC_SAMPLER_DESC> samplers;
            D3D12_ROOT_SIGNATURE_FLAGS flags;
        };

        Com<ID3DBlob> compile(D3D_ROOT_SIGNATURE_VERSION version, const Create& create);
    }
}
