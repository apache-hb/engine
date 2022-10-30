#include "engine/graph/resource.h"
#include "engine/graph/render.h"

using namespace simcoe;
using namespace simcoe::graph;

CommandResource::CommandResource(const Info& info, rhi::Device& device, rhi::CommandList::Type type)
    : RenderResource(info)
{
    size_t buffers = getParent()->getInfo().backBuffers;

    allocators = new rhi::Allocator[buffers];
    for (size_t i = 0; i < buffers; ++i) {
        allocators[i] = device.newAllocator(type);
    }

    commands = device.newCommandList(allocators[0], type);
}
