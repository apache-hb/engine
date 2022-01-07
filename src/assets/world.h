#pragma once

#include "mesh.h"
#include "texture.h"

namespace engine::assets {
    struct Mesh {
        size_t buffer;
        size_t texture;
    };

    struct World {
        std::vector<VertexBuffer> vertexBuffers;
        std::vector<IndexBuffer> indexBuffers;
        std::vector<Texture> textures;

        std::vector<IndexBufferView> indexBufferViews;

        std::vector<Mesh> meshes;
    };
}
