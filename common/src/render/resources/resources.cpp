#include "engine/render/resources/resources.h"

using namespace simcoe;
using namespace simcoe::render;

namespace {
    constexpr math::float4 kClearColour = { 0.f, 0.2f, 0.4f, 1.f };
    constexpr math::float4 kLetterBox = { 0.f, 0.f, 0.f, 1.f };
}

BufferResource::BufferResource(const Info& info, size_t handle)
    : Resource(info)
    , cbvOffset(handle)
{ }

void BufferResource::addBarrier(Barriers& barriers, Output* output, Input* input) {
    Resource::addBarrier(barriers, output, input);

    auto before = output->getState();
    auto after = input->getState();

    ASSERT(before != State::eInvalid && after != State::eInvalid);
    if (before == after) { return; }

    barriers.push_back(rhi::newStateTransition(get(), before, after));
}

RenderTargetResource::RenderTargetResource(const Info& info)
    : BufferResource(info, SIZE_MAX)
{ }

TextureResource::TextureResource(const Info& info, State initial, rhi::TextureSize size, bool hostVisible)
    : BufferResource(info, ::getContext(info).getHeap().alloc(DescriptorSlot::eTexture))
{ 
    auto& ctx = getContext();
    auto& heap = ctx.getHeap();
    auto& device = ctx.getDevice();

    const auto desc = rhi::newTextureDesc(size, initial);
    const auto visibility = hostVisible ? rhi::DescriptorSet::Visibility::eHostVisible : rhi::DescriptorSet::Visibility::eDeviceOnly;

    buffer = device.newTexture(desc, visibility, kClearColour);
    device.createTextureBufferView(get(), heap.cpuHandle(cbvHandle(), 1, DescriptorSlot::eTexture));

    buffer.rename(getName());
}

SceneTargetResource::SceneTargetResource(const Info& info, State initial)
    : TextureResource(info, initial, render::getContext(info).sceneSize(), false)
{ 
    auto& ctx = getContext();
    auto& device = ctx.getDevice();

    rtvOffset = ctx.getPresentQueue().getSceneHandle();
    device.createRenderTargetView(get(), rtvOffset);
}
