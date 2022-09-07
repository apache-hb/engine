#include "commands.h"

namespace engine::render {
    Commands::Commands(ID3D12Device* device, D3D12_COMMAND_LIST_TYPE type, ID3D12CommandAllocator* allocator, PipelineState state) {
        check(device->CreateCommandList(0, type, allocator, state.get(), IID_PPV_ARGS(addr())), "failed to create command list");
    }

    void Commands::reset(ID3D12CommandAllocator* allocator, PipelineState state) {
        check(get()->Reset(allocator, state.get()), "failed to reset command list");
    }

    void Commands::close() {
        check(get()->Close(), "failed to close command list");
    }
}
