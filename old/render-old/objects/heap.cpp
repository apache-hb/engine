#include "heap.h"

namespace engine::render {
    DescriptorHeap::DescriptorHeap(ID3D12Device *device, std::wstring_view name, const D3D12_DESCRIPTOR_HEAP_DESC& desc) {
        check(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(addr())), "failed to create descriptor heap");
        descriptorSize = device->GetDescriptorHandleIncrementSize(desc.Type);
        rename(name);
    }
}
