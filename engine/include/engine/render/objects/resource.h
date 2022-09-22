#pragma once

#include "dx/d3d12.h"
#include "engine/render/util.h"

namespace engine::render::d3d12 {
    template<typename T>
    concept IsResource = std::is_base_of_v<ID3D12Resource, T>;

    template<IsResource T = ID3D12Resource>
    struct Resource : Object<T> {
        using Super = Object<T>;
        using Super::Super;

        D3D12_GPU_VIRTUAL_ADDRESS gpuAddress() {
            return Super::get()->GetGPUVirtualAddress();
        }
    };

    template<IsResource T = ID3D12Resource>
    struct VertexBuffer : Resource<T> {
        using Super = Resource<T>;

        VertexBuffer() : Super(nullptr) { }

        VertexBuffer(T *resource, UINT size, UINT stride) 
            : Super(resource)
            , view({ Super::gpuAddress(), size, stride }) 
        { }

        D3D12_VERTEX_BUFFER_VIEW view;
    };

    template<IsResource T = ID3D12Resource>
    struct IndexBuffer : Resource<T> {
        using Super = Resource<T>;

        IndexBuffer() : Super(nullptr) { }

        IndexBuffer(T *resource, UINT size, DXGI_FORMAT format) 
            : Super(resource)
            , view({ Super::gpuAddress(), size, format }) 
        { }

        D3D12_INDEX_BUFFER_VIEW view;
    };
}
