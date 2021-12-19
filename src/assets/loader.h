#pragma once

#include "util/error.h"
#include <vector>
#include <span>
#include <string_view>
#include <windows.h>
#include <DirectXMath.h>

namespace engine::loader {
    using namespace DirectX;

    struct Vertex {
        XMFLOAT3 position;
        XMFLOAT3 normal;
        XMFLOAT2 texcoord;
    };

    struct Model {
        std::vector<Vertex> vertices;
        std::vector<DWORD> indices;
    };

    struct Texture {
        size_t width;
        size_t height;
        size_t bpp;

        std::vector<UINT8> pixels;
    };

    struct Scene {
        // all verticies that make up the scene
        std::vector<Vertex> vertices;

        // all indicies into the verticies
        std::vector<DWORD> indices;

        std::vector<Texture> textures;

        struct Object {
            size_t offset; // offset into the indices
            size_t length; // number of indicies
            size_t texture; // index into the texture buffer
        };

        std::vector<Object> objects;
    };

    struct LoadError : engine::Error {
        LoadError(std::string error, std::string warn, std::source_location location = std::source_location::current())
            : engine::Error(error, location)
            , warning(warn)
        { }

    private:
        std::string warning;
    };

    Scene objScene(std::string_view path);

    Texture image(std::string_view path);
}
