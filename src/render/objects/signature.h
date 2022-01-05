#pragma once

#include "render/util.h"

namespace engine::render {
    namespace visibility {
        constexpr D3D12_SHADER_VISIBILITY kAll = D3D12_SHADER_VISIBILITY_ALL;
        constexpr D3D12_SHADER_VISIBILITY kPixel = D3D12_SHADER_VISIBILITY_PIXEL;
        constexpr D3D12_SHADER_VISIBILITY kVertex = D3D12_SHADER_VISIBILITY_VERTEX;
    }

    constexpr CD3DX12_DESCRIPTOR_RANGE1 cbvRange(UINT reg, UINT count, UINT space = 0, D3D12_DESCRIPTOR_RANGE_FLAGS flags = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC) {
        return CD3DX12_DESCRIPTOR_RANGE1({
            .RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV,
            .NumDescriptors = count,
            .BaseShaderRegister = reg,
            .RegisterSpace = space,
            .Flags = flags,
            .OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND
        });
    }

    constexpr CD3DX12_DESCRIPTOR_RANGE1 srvRange(UINT reg, UINT count, UINT space = 0, D3D12_DESCRIPTOR_RANGE_FLAGS flags = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE) {
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

    constexpr CD3DX12_ROOT_PARAMETER1 cbvParameter(D3D12_SHADER_VISIBILITY visibility, UINT reg, UINT space = 0) {
        return CD3DX12_ROOT_PARAMETER1({
            .ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV,
            .Descriptor = {
                .ShaderRegister = reg,
                .RegisterSpace = space,
            },
            .ShaderVisibility = visibility
        });
    }

    constexpr CD3DX12_ROOT_PARAMETER1 srvParameter(D3D12_SHADER_VISIBILITY visibility, UINT reg, UINT space = 0) {
        return CD3DX12_ROOT_PARAMETER1({
            .ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV,
            .Descriptor = {
                .ShaderRegister = reg,
                .RegisterSpace = space,
            },
            .ShaderVisibility = visibility
        });
    }

    struct RootCreate {
        std::span<const CD3DX12_ROOT_PARAMETER1> params;
        std::span<const D3D12_STATIC_SAMPLER_DESC> samplers;
        D3D12_ROOT_SIGNATURE_FLAGS flags;
    };

    Com<ID3DBlob> compileRootSignature(D3D_ROOT_SIGNATURE_VERSION version, const RootCreate& info);
}
