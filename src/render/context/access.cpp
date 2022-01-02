#include "render/render.h"

using namespace engine;
using namespace engine::render;

Factory& Context::getFactory() {
    return *info.factory;
}

Adapter& Context::getAdapter() {
    return getFactory().getAdapter(info.adapter);
}

Device<ID3D12Device4>& Context::getDevice() {
    return device;
}

system::Window& Context::getWindow() {
    return *info.window;
}

Resolution Context::getDisplayResolution() const {
    return info.resolution;
}

Resolution Context::getInternalResolution() {
    auto [width, height] = getWindow().getClientSize();
    return { UINT(width), UINT(height) };
}

DXGI_FORMAT Context::getFormat() const {
    return DXGI_FORMAT_R8G8B8A8_UNORM;
}

UINT Context::getBufferCount() const {
    return info.buffers;
}

UINT Context::getCurrentFrame() const {
    return frameIndex;
}

Object<ID3D12CommandAllocator>& Context::getAllocator(Allocator::Index index) {
    const auto frame = getCurrentFrame();
    return frameData[frame].allocators[size_t(index)];
}

Object<ID3D12Resource> Context::getTarget() {
    const auto frame = getCurrentFrame();
    return frameData[frame].target;
}

UINT Context::requiredRtvHeapSize() const {
    return getBufferCount() + 1; // +1 for the intermediate scene target
}

UINT Context::requiredCbvHeapSize() const {
    return Resources::Total;
}

D3D12_CPU_DESCRIPTOR_HANDLE Context::sceneTargetRtvCpuHandle() {
    return rtvHeap.cpuHandle(0); // 0 is the intermediate scene target
}

D3D12_CPU_DESCRIPTOR_HANDLE Context::sceneTargetCbvCpuHandle() {
    return cbvHeap.cpuHandle(Resources::SceneTarget);
}

D3D12_CPU_DESCRIPTOR_HANDLE Context::rtvHeapCpuHandle(UINT index) {
    return rtvHeap.cpuHandle(index + 1); // +1 is the intermediate scene target
}
