#pragma once

#include "pipeline.h"

namespace engine::render {
    namespace commands {
        constexpr auto kDirect = D3D12_COMMAND_LIST_TYPE_DIRECT;
        constexpr auto kCopy = D3D12_COMMAND_LIST_TYPE_COPY;
        constexpr auto kBundle = D3D12_COMMAND_LIST_TYPE_BUNDLE;
    }

    struct Commands : Object<ID3D12GraphicsCommandList> {
        using Super = Object<ID3D12GraphicsCommandList>;

        Commands() = default;
        Commands(ID3D12Device* device, D3D12_COMMAND_LIST_TYPE type, ID3D12CommandAllocator* allocator, PipelineState state);

        void reset(ID3D12CommandAllocator* allocator, PipelineState state);
        void close();
    };

    struct Bundle : Commands {
        using Super = Commands;

        Bundle() = default;
        Bundle(ID3D12Device* device, ID3D12CommandAllocator* allocator, PipelineState state)
            : Super(device, commands::kBundle, allocator, state)
        { }

        template<typename F>
        Bundle& record(F&& func) {
            func(*this);
            close();
            return *this;
        }
    };
}
