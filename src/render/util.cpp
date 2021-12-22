#include "util.h"
#include "debug/debug.h"

#include "util/strings.h"

#include <comdef.h>
#include <d3dcompiler.h>

extern "C" {
    __declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
    __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}

namespace engine::render {
    std::string toString(HRESULT hr) {
        return strings::encode(_com_error(hr).ErrorMessage());
    }

    void check(HRESULT hr, const std::string& message, std::source_location location) {
        if (FAILED(hr)) { throw render::Error(hr, message, location); }
    }

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
}
