#pragma once

#include "render/objects/device.h"
#include "assets/loader.h"

namespace engine::render {
    struct Context;
}

namespace engine::assets {
    struct Primitive {
        std::vector<D3D12_VERTEX_BUFFER_VIEW> views;
        UINT vertexCount;

        D3D12_PRIMITIVE_TOPOLOGY topology;

        render::Object<ID3D12RootSignature> rootSignature;
        render::Object<ID3D12PipelineState> pipelineState;
    };

    struct IndexedPrimitive : Primitive {
        D3D12_INDEX_BUFFER_VIEW indexView;
        UINT indexCount;
    };

    struct Mesh {
        std::vector<Primitive> primitives;
        std::vector<IndexedPrimitive> indexedPrimitives;
    };

    struct Image {
        render::Resource texture;
    };

    struct Scene {
        Scene(render::Context* ctx, std::string_view path);

    private:
        void loadImages();

        render::Context* context;
        std::string_view name;

        assets::gltf::Model model;

        render::DescriptorHeap srvHeap;

        std::vector<Image> images;
        std::vector<Mesh> meshes;
    };
}
