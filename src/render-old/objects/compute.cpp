#include "compute.h"

namespace engine::render {
    ComputeLibrary::ComputeLibrary(Create info): create(info) {
        csBlob = compileShader(create.path, create.csMain, "cs_5_0");
    }

    CD3DX12_SHADER_BYTECODE ComputeLibrary::shader() {
        return CD3DX12_SHADER_BYTECODE(csBlob.get());
    }
}
