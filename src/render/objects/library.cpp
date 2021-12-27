#include "library.h"
#include "render/debug/debug.h"
#include <d3dcompiler.h>

namespace engine::render {
    Com<ID3DBlob> compileShader(std::wstring_view path, std::string_view entry, std::string_view target) {
        Com<ID3DBlob> shader;
        Com<ID3DBlob> error;

        HRESULT hr = D3DCompileFromFile(path.data(), nullptr, nullptr, entry.data(), target.data(), DEFAULT_SHADER_FLAGS, 0, &shader, &error);
        if (FAILED(hr)) {
            auto msg = error.valid() ? (const char*)error->GetBufferPointer() : "no error message";
            throw render::Error(hr, std::format("failed to compile shader {} {}\n{}", entry, target, msg));
        }

        return shader;
    }

    ShaderLibrary::ShaderLibrary(const CompileCreate& create) {
        inputs = create.layout;
        vsBytecode = compileShader(create.path, create.vsMain, "vs_5_0");
        psBytecode = compileShader(create.path, create.psMain, "ps_5_0");
    }

    D3D12_INPUT_LAYOUT_DESC ShaderLibrary::layout() const {
        return { inputs.data(), UINT(inputs.size()) };
    }

    CD3DX12_SHADER_BYTECODE ShaderLibrary::vertex() {
        return CD3DX12_SHADER_BYTECODE(vsBytecode.get());
    }

    CD3DX12_SHADER_BYTECODE ShaderLibrary::pixel() {
        return CD3DX12_SHADER_BYTECODE(psBytecode.get());
    }
}
