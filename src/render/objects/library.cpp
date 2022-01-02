#include "library.h"

#include "render/debug/debug.h"

using namespace engine::render;

Com<ID3DBlob> compileShader(std::wstring_view path, std::string_view entry, std::string_view target) {
    Com<ID3DBlob> shader;
    Com<ID3DBlob> error;

    HRESULT hr = D3DCompileFromFile(path.data(), nullptr, nullptr, entry.data(), target.data(), kShaderFlags, 0, &shader, &error);
    if (FAILED(hr)) {
        auto msg = error.valid() ? (const char*)error->GetBufferPointer() : "no error message";
        throw Error(hr, std::format("failed to compile shader {} {}\n{}", entry, target, msg));
    }

    return shader;
}

ShaderLibrary::ShaderLibrary(Create create): create(create) {
    auto path = create.path;

    vsBlob = compileShader(path, create.vsMain, "vs_5_1");
    psBlob = compileShader(path, create.psMain, "ps_5_1");
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
