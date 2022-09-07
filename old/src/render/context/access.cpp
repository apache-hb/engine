#include "render/context.h"

#include "render/objects/factory.h"

using namespace engine::render;

Factory& Context::getFactory() {
    return *info.factory;
}

Adapter& Context::getAdapter() {
    return getFactory().getAdapter(info.adapter);
}

engine::system::Window& Context::getWindow() {
    return *info.window;
}

DXGI_FORMAT Context::getFormat() {
    return DXGI_FORMAT_R8G8B8A8_UNORM;
}

UINT Context::getBufferCount() {
    return info.buffers;
}

Resolution Context::getSceneResolution() {
    return info.resolution;
}

Resolution Context::getPostResolution() {
    auto [width, height] = getWindow().getClientSize();
    return Resolution(width, height);
}


DescriptorHeap::CpuHandle Context::rtvCpuHandle(UINT index) {
    return rtvHeap.cpuHandle(Targets::Total + index);
}
