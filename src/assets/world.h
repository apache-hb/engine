#pragma once

#include "mesh.h"
#include "texture.h"

namespace engine::assets {
    struct Material {
        size_t texture;
        size_t sampler;
    };

    struct Mesh {
        std::vector<IndexBufferView> views;
        float4x4 transform;

        size_t buffer;
        Material material;
    };

    struct World {
        std::vector<VertexBuffer> vertexBuffers;
        std::vector<IndexBuffer> indexBuffers;
        
        std::vector<Texture> textures;
        std::vector<Sampler> samplers;

        std::vector<Mesh> meshes;
    };
}
