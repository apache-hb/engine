#pragma once

#include "render/util.h"

namespace engine::render {
    struct Resource : Object<ID3D12Resource> {
        using Super = Object<ID3D12Resource>;
        using Super::Super;
    };

    struct VertexBuffer {
        Resource resource;
        D3D12_VERTEX_BUFFER_VIEW view;

        void tryDrop(std::string_view name = "") {
            resource.tryDrop(name);
        }
    };

    struct IndexBuffer {
        Resource resource;
        D3D12_INDEX_BUFFER_VIEW view;

        void tryDrop(std::string_view name = "") {
            resource.tryDrop(name);
        }
    };
}
