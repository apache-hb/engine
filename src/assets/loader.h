#pragma once

#include "util/error.h"
#include <vector>
#include <span>
#include <string_view>
#include <source_location>

#include "gltf/tinygltf.h"

namespace engine::loader {
    namespace gltf = tinygltf;

    struct Texture {
        size_t width;
        size_t height;
        size_t bpp;

        std::vector<uint8_t> pixels;
    };

    struct LoadError : engine::Error {
        LoadError(std::string error, std::string warn, std::source_location location = std::source_location::current())
            : engine::Error(error, location)
            , warning(warn)
        { }

    private:
        std::string warning;
    };

    gltf::Model gltfScene(std::string_view path);

    Texture image(std::string_view path);
}
