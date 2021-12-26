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

    Object<ID3D12Resource>& Context::getIntermediateTarget() {
        return intermediateRenderTarget;
    }

    CD3DX12_CPU_DESCRIPTOR_HANDLE Context::getIntermediateRtvHandle() {
        return rtvHeap.cpuHandle(0);
    }

    CD3DX12_CPU_DESCRIPTOR_HANDLE Context::getIntermediateCbvHandle() {
        return cbvHeap.cpuHandle(CbvResources::Intermediate);
    }
}
