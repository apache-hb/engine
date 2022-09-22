#pragma once

#include "dx/d3dx12.h"
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

    template<typename T, IsResource R = ID3D12Resource>
    struct ConstBuffer : Resource<R> {
        using Super = Resource<R>;

        ConstBuffer() : Super(nullptr), data(nullptr) { }

        ConstBuffer(R *resource, T *data) 
            : Super(resource)
            , data(data)
        {
            CD3DX12_RANGE range(0, 0);
            CHECK(resource->Map(0, &range, &ptr));
            update();
        }

        void update() {
            memcpy(ptr, data, sizeof(T));
        }

        T *data;
    private:
        void *ptr;
    };
}
