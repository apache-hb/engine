#include "library.h"

namespace engine::render {
    ShaderLibrary::ShaderLibrary(Create create): create(create) {
        auto path = create.path;
        vsBlob = compileShader(path, create.vsMain, "vs_5_0");
        psBlob = compileShader(path, create.psMain, "ps_5_0");
    }

    CD3DX12_SHADER_BYTECODE ShaderLibrary::vertex() {
        return CD3DX12_SHADER_BYTECODE(vsBlob.get());
    }

    CD3DX12_SHADER_BYTECODE ShaderLibrary::pixel() {
        return CD3DX12_SHADER_BYTECODE(psBlob.get());
    }

    D3D12_INPUT_LAYOUT_DESC ShaderLibrary::layout() const {
        auto& layout = create.layout;
        return { layout.data(), UINT(layout.size()) };
    }
}
