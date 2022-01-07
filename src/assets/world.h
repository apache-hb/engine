#pragma once

#include "mesh.h"
#include "texture.h"

namespace engine::assets {
    struct TexturedMesh {
        Mesh mesh;
        size_t texture;
    };

    struct World {
        std::vector<TexturedMesh> meshes;
        std::vector<Texture> textures;
    };
}
