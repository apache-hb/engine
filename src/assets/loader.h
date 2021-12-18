#pragma once

#include "util/error.h"
#include <vector>
#include <string_view>
#include <windows.h>
#include <DirectXMath.h>

namespace engine::loader {
    using namespace DirectX;

    struct Vertex {
        XMFLOAT3 position;
        XMFLOAT2 texcoord;
    };

    struct Model {
        std::vector<Vertex> vertices;
        std::vector<DWORD> indices;
    };

    struct LoadError : engine::Error {
        LoadError(std::string error, std::string warn, std::source_location location = std::source_location::current())
            : engine::Error(error, location)
            , warning(warn)
        { }

    private:
        std::string warning;
    };

    Model obj(std::string_view path);
}
