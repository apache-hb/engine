#pragma once

#include <span>
#include <string_view>
#include "render/util.h"

namespace engine::render {
    using ShaderInput = const D3D12_INPUT_ELEMENT_DESC;

    constexpr ShaderInput shaderInput(std::string_view name, 
                                      DXGI_FORMAT format, 
                                      UINT offset = D3D12_APPEND_ALIGNED_ELEMENT,
                                      D3D12_INPUT_CLASSIFICATION classification = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA) 
    {
        return { name.data(), 0, format, 0, offset, classification, 0 };
    }

    void enableSm6();

    struct ShaderLibrary {
        struct Create {
            std::wstring_view path;

            std::string_view vsMain;
            std::string_view psMain;

            std::span<ShaderInput> layout;
        };

        struct CreateBinary {
            std::wstring_view vsPath;
            std::wstring_view psPath;

            std::span<ShaderInput> layout;
        };

        ShaderLibrary() = default;
        ShaderLibrary(Create create);
        ShaderLibrary(CreateBinary create);

        D3D12_INPUT_LAYOUT_DESC layout() const;

        CD3DX12_SHADER_BYTECODE vertex();
        CD3DX12_SHADER_BYTECODE pixel();

    private:
        std::span<ShaderInput> inputs;
        Com<ID3DBlob> vsBlob;
        Com<ID3DBlob> psBlob;
    };
}