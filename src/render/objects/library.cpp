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

Com<ID3DBlob> loadShader(std::wstring_view path) {
    Com<ID3DBlob> shader;

    HRESULT hr = D3DReadFileToBlob(path.data(), &shader);
    check(hr, std::format(L"failed to load shader {}", path));

    return shader;
}

constexpr GUID sm6Id = { /* 76f5573e-f13a-40f5-b297-81ce9e18933f */
    0x76f5573e,
    0xf13a,
    0x40f5,
    { 0xb2, 0x97, 0x81, 0xce, 0x9e, 0x18, 0x93, 0x3f }
};

void engine::render::enableSm6() {
    HRESULT hr = D3D12EnableExperimentalFeatures(1, &sm6Id, nullptr, nullptr);
    check(hr, "failed to enable sm6");
}

ShaderLibrary::ShaderLibrary(Create create): inputs(create.layout) {
    auto path = create.path;

    vsBlob = compileShader(path, create.vsMain, "vs_5_1");
    psBlob = compileShader(path, create.psMain, "ps_5_1");
}

ShaderLibrary::ShaderLibrary(CreateBinary create): inputs(create.layout) {
    vsBlob = loadShader(create.vsPath);
    psBlob = loadShader(create.psPath);
}

CD3DX12_SHADER_BYTECODE ShaderLibrary::vertex() {
    return CD3DX12_SHADER_BYTECODE(vsBlob.get());
}

CD3DX12_SHADER_BYTECODE ShaderLibrary::pixel() {
    return CD3DX12_SHADER_BYTECODE(psBlob.get());
}

D3D12_INPUT_LAYOUT_DESC ShaderLibrary::layout() const {
    return { inputs.data(), UINT(inputs.size()) };
}
