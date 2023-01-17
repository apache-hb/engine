#pragma once

#include <vector>
#include <string>

namespace editor::shaders {
    enum Target {
        eVertexShader,
        ePixelShader
    };

    void init();

    std::vector<std::byte> compile(const std::string& source, Target target, bool debug);
}
