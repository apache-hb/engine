#include "util.h"

namespace render::d3d12 {
    HRESULT BundleCommandList::create(
        Device1 device,
        CommandAllocator allocator,
        PipelineState state,
        BundleCommandList::Self **out
    )
    {
        return device->CreateCommandList(0,
            D3D12_COMMAND_LIST_TYPE_BUNDLE,
            allocator.get(),
            state.get(),
            IID_PPV_ARGS(out)  
        );
    }

    HRESULT DirectCommandList::create(
        Device1 device,
        CommandAllocator allocator,
        PipelineState state,
        DirectCommandList::Self **out
    )
    {
        return device->CreateCommandList(0,
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            allocator.get(),
            state.get(),
            IID_PPV_ARGS(out)  
        );
    }
}
