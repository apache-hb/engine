#pragma once

#include "render/util.h"

namespace engine::render {
    namespace visibility {
        constexpr D3D12_SHADER_VISIBILITY All = D3D12_SHADER_VISIBILITY_ALL;
        constexpr D3D12_SHADER_VISIBILITY Pixel = D3D12_SHADER_VISIBILITY_PIXEL;
        constexpr D3D12_SHADER_VISIBILITY Vertex = D3D12_SHADER_VISIBILITY_VERTEX;
    }

    constexpr CD3DX12_DESCRIPTOR_RANGE1 cbvRange(UINT reg, UINT count, UINT space = 0, D3D12_DESCRIPTOR_RANGE_FLAGS flags = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC) {
        return CD3DX12_DESCRIPTOR_RANGE1(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, count, reg, space, flags);
    }

    constexpr CD3DX12_DESCRIPTOR_RANGE1 srvRange(UINT reg, UINT count, UINT space = 0, D3D12_DESCRIPTOR_RANGE_FLAGS flags = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC) {
        return CD3DX12_DESCRIPTOR_RANGE1(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, count, reg, space, flags);
    }

    constexpr CD3DX12_ROOT_PARAMETER1 tableParameter(D3D12_SHADER_VISIBILITY visibility, std::span<const CD3DX12_DESCRIPTOR_RANGE1> ranges) {
        return CD3DX12_ROOT_PARAMETER1({
            .ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
            .DescriptorTable = {
                .NumDescriptorRanges = UINT(ranges.size()),
                .pDescriptorRanges = ranges.data(),
            },
            .ShaderVisibility = visibility
        });
    }

    constexpr CD3DX12_ROOT_PARAMETER1 root32BitParameter(D3D12_SHADER_VISIBILITY visibility, UINT reg, UINT count, UINT space = 0) {
        return CD3DX12_ROOT_PARAMETER1({
            .ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS,
            .Constants = {
                .ShaderRegister = reg,
                .RegisterSpace = space,
                .Num32BitValues = count,
            },
            .ShaderVisibility = visibility
        });
    }
}
