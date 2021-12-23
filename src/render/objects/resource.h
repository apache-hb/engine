#pragma once

#include "render/util.h"

namespace engine::render {
    struct Resource : Object<ID3D12Resource> {
        using Super = Object<ID3D12Resource>;
        using Super::Super;
    };

    struct VertexBuffer : Resource {
        VertexBuffer() = default;
        VertexBuffer(Resource res, D3D12_VERTEX_BUFFER_VIEW view)
            : Resource(res)
            , view(view)
        { }

        D3D12_VERTEX_BUFFER_VIEW view;
    };

    struct IndexBuffer : Resource {
        IndexBuffer() = default;
        IndexBuffer(Resource res, D3D12_INDEX_BUFFER_VIEW view)
            : Resource(res)
            , view(view)
        { }

        D3D12_INDEX_BUFFER_VIEW view;
    };
}
