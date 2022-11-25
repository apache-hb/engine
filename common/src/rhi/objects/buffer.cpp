#include "engine/rhi/rhi.h"
#include "objects/common.h"

using namespace simcoe;
using namespace simcoe::rhi;

void Buffer::write(const void *src, size_t size) {
    void *ptr = map();
    memcpy(ptr, src, size);
    unmap();
}

void Buffer::writeTexture(const void* src, rhi::TextureSize size) {
    ID3D12Device *pDevice = nullptr;
    HR_CHECK(get()->GetDevice(IID_PPV_ARGS(&pDevice)));

    D3D12_RESOURCE_DESC desc = get()->GetDesc();

    UINT rowCount = 0;
    UINT64 rowSize = 0;
    pDevice->GetCopyableFootprints(&desc, 0, 1, 0,nullptr, &rowCount, &rowSize, nullptr);

    std::byte* ptr = (std::byte*)map();

    for (UINT i = 0; i < rowCount; i++) {
        std::byte* dst = ptr + rowSize * i;
        std::byte* it = (std::byte*)src + size.width * 4 * i; // TODO: may not have 4 channels always
        memcpy(dst, it, size.width * 4);
    }

    unmap();
}

void *Buffer::map() {
    void *ptr;
    CD3DX12_RANGE range(0, 0);
    HR_CHECK(get()->Map(0, &range, &ptr));
    return ptr;
}

void Buffer::unmap() {
    CD3DX12_RANGE range(0, 0);
    get()->Unmap(0, &range);
}

rhi::GpuHandle Buffer::gpuAddress() {
    return rhi::GpuHandle(get()->GetGPUVirtualAddress());
}
