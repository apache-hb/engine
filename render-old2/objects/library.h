#pragma once

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

    struct ShaderLibrary {
        using Inputs = std::span<ShaderInput>;

        /**
         * create a shader library from a source file and compile it at runtime
         */
        struct CompileCreate {
            std::wstring_view path; // path to the shader source file

            LPCSTR vsMain; // name of the vertex shader main function, NULL if not used
            LPCSTR psMain; // name of the pixel shader main function, NULL if not used

            Inputs layout; // input layout description
        };

        /**
         * create a shader library from prebuilt bytecode
         */
        struct BuiltCreate {
            LPWSTR vsPath; /// path to the vertex shader, NULL if not used
            LPWSTR psPath; /// path to the pixel shader, NULL if not used
        };

        ShaderLibrary() = default;
        ShaderLibrary(const CompileCreate& create);
        ShaderLibrary(const BuiltCreate& create);

        D3D12_INPUT_LAYOUT_DESC layout() const;

        CD3DX12_SHADER_BYTECODE vertex();
        CD3DX12_SHADER_BYTECODE pixel();

    private:
        Inputs inputs;

        Com<ID3DBlob> vsBytecode;
        Com<ID3DBlob> psBytecode;
    };
}
