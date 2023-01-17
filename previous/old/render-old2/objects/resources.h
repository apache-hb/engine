#pragma once

#include "render/util.h"

namespace engine::render {
    struct Resource : Object<ID3D12Resource> {
        using Super = Object<ID3D12Resource>;
        
        Resource() = default;

        void *map(UINT subResource, D3D12_RANGE *range = nullptr) {
            void *data;
            check(get()->Map(subResource, range, &data), "failed to map resource");
            return data;
        }

        void unmap(UINT subResource, D3D12_RANGE *range = nullptr) {
            get()->Unmap(subResource, range);
        }

        D3D12_GPU_VIRTUAL_ADDRESS gpuAddress() {
            return get()->GetGPUVirtualAddress();
        }
    };

    struct VertexBuffer {
        Object<ID3D12Resource> resource;
        D3D12_VERTEX_BUFFER_VIEW view;
    };
}
