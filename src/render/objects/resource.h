#pragma once

#include "render/util.h"

namespace engine::render {
    struct Resource : Object<ID3D12Resource> {
        using Super = Object<ID3D12Resource>;
        using Super::Super;

        void* map(UINT subresource, D3D12_RANGE* range = nullptr);
        void unmap(UINT subresource, D3D12_RANGE* range = nullptr);

        template<typename T>
        void write(UINT subresource, const std::span<T> data) {
            writeBytes(subresource, data.data(), data.size() * sizeof(T));
        }

        void writeBytes(UINT subresource, const void* data, size_t size);
    };
}
