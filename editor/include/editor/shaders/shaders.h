#pragma once

#include <vector>
#include <string>

namespace editor::shaders {
    enum Target {
        eVertexShader,
        ePixelShader
    };

    std::vector<std::byte> compile(const std::string& source, Target target, bool debug);
}
