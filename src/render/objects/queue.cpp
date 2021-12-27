#include "queue.h"

namespace engine::render {
    Queue::Queue(ID3D12Device* device, const D3D12_COMMAND_QUEUE_DESC& desc) {
        check(device->CreateCommandQueue(&desc, IID_PPV_ARGS(addr())), "failed to create command queue");
    }

    void Queue::execute(std::span<Commands> all) {
        UINT size = UINT(all.size());
        auto** lists = reinterpret_cast<ID3D12CommandList**>(alloca(size * sizeof(ID3D12CommandList*)));
        for (size_t i = 0; i < size; ++i) {
            lists[i] = reinterpret_cast<ID3D12CommandList*>(all[i].get());
        }

        Super::get()->ExecuteCommandLists(size, lists);
    }
}
