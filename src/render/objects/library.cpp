#include "library.h"

namespace engine::render {
    std::vector<D3D_SHADER_MACRO> collectDefines(const std::map<std::string_view, std::string_view>& defines) {
        std::vector<D3D_SHADER_MACRO> result;

        for (const auto& [key, val] : defines) {
            result.push_back({ key.data(), val.data() });
        }

        result.push_back({ nullptr, nullptr });
        return result;
    }

    ShaderLibrary::ShaderLibrary(Create create, const std::map<std::string_view, std::string_view>& defines): create(create) {
        auto path = create.path;
        auto macros = collectDefines(defines);
        
        vsBlob = compileShader(path, create.vsMain, "vs_5_1", macros);
        psBlob = compileShader(path, create.psMain, "ps_5_1", macros);
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
