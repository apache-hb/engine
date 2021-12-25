#pragma once

#include "render/util.h"

namespace engine::render {
    struct Resource : Object<ID3D12Resource> {
        using Super = Object<ID3D12Resource>;
        using Super::Super;

        void *map(UINT subResource, D3D12_RANGE *range = nullptr) {
            void *data;
            check(get()->Map(subResource, range, &data), "failed to map resource");
            return data;
        }

        void unmap(UINT subResource, D3D12_RANGE *range = nullptr) {
            get()->Unmap(subResource, range);
        }
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
