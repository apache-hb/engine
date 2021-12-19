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

        std::span<UINT8> pixels;
    };

    struct Scene {
        struct Item {
            Model model;
            Texture texture;
        };

        std::vector<Item> models;
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
    Scene objScene(std::string_view path);

    Texture tga(std::string_view path);
}
