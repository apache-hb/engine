#pragma once

#include "util/error.h"
#include <vector>
#include <span>
#include <string_view>
#include <source_location>

#include "gltf/tinygltf.h"

namespace engine::assets {
    namespace gltf = tinygltf;

    struct LoadError : engine::Error {
        LoadError(std::string error, std::string warn, std::source_location location = std::source_location::current())
            : engine::Error(error, location)
            , warning(warn)
        { }

    private:
        std::string warning;
    };

    gltf::Model gltfScene(std::string_view path);
}
