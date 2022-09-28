#include "objects/allocator.h"

using namespace engine;

DxAllocator::DxAllocator(ID3D12CommandAllocator *allocator)
    : allocator(allocator)
{ }

void DxAllocator::reset() {
    DX_CHECK(allocator->Reset());
}

ID3D12CommandAllocator *DxAllocator::get() { 
    return allocator.get(); 
}
