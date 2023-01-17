#pragma once

#include "render/util.h"

namespace engine::render {
    struct ComputeLibrary {
        struct Create {
            std::wstring_view path;

            std::string_view csMain;
        };

        ComputeLibrary() = default;
        ComputeLibrary(Create info);

        CD3DX12_SHADER_BYTECODE shader();

    private:
        Create create;

        Com<ID3DBlob> csBlob;
    };
}
