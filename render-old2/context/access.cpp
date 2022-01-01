#include "render/render.h"

namespace engine::render {
    Factory& Context::getFactory() {
        return info.factory;
    }

    Adapter& Context::getAdapter() {
        return getFactory().getAdapter(info.currentAdapter);
    }

    system::Window& Context::getWindow() {
        return *info.window;
    }

    Resolution Context::getCurrentResolution() const {
        return info.resolution;
    }

    DXGI_FORMAT Context::getFormat() const {
        return DXGI_FORMAT_R8G8B8A8_UNORM; // this will change once we get hdr support
    }

    UINT Context::getBackBufferCount() const {
        return info.backBuffers;
    }



    ///
    /// calculated values
    ///

    UINT Context::requiredRtvHeapSize() const {
        return getBackBufferCount() + 1; // +1 for the intermediate target
    }

    UINT Context::requiredCbvHeapSize() const {
        return CbvResources::Total;
    }

    UINT Context::getCurrentFrameIndex() const {
        return frameIndex;
    }

    Resource& Context::getIntermediateTarget() {
        return intermediateRenderTarget;
    }

    CD3DX12_CPU_DESCRIPTOR_HANDLE Context::getIntermediateRtvHandle() {
        return rtvHeap.cpuHandle(0);
    }

    CD3DX12_CPU_DESCRIPTOR_HANDLE Context::getIntermediateCbvCpuHandle() {
        return cbvHeap.cpuHandle(CbvResources::Intermediate);
    }
    
    CD3DX12_GPU_DESCRIPTOR_HANDLE Context::getIntermediateCbvGpuHandle() {
        return cbvHeap.gpuHandle(CbvResources::Intermediate);
    }

    CD3DX12_CPU_DESCRIPTOR_HANDLE Context::getRenderTargetCpuHandle(UINT index) {
        return rtvHeap.cpuHandle(index + 1); // +1 for the intermediate target
    }

    CD3DX12_GPU_DESCRIPTOR_HANDLE Context::getRenderTargetGpuHandle(UINT index) {
        return rtvHeap.gpuHandle(index + 1); // +1 for the intermediate target
    }


    Object<ID3D12CommandAllocator> Context::getAllocator(Allocator::Index type, size_t index) {
        UINT idx = index == SIZE_MAX ? getCurrentFrameIndex() : UINT(index);
        return frameData[idx].allocators[type];
    }
}
