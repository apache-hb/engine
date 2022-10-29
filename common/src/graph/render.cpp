#include "engine/graph/render.h"
#include "engine/rhi/rhi.h"

using namespace simcoe;
using namespace simcoe::graph;

RenderGraph::RenderGraph(const Create& create) 
    : info(create) 
    , cbvAlloc(create.heapSize)
{
    device = rhi::getDevice();

    // create presentation resources
    queue = device.newQueue(rhi::CommandList::Type::eDirect);
    swapchain = queue.newSwapChain(info.window, info.backBuffers);

    // create rtv heap
    rtvHeap = device.newDescriptorSet(totalRtvSlots(), rhi::DescriptorSet::Type::eRenderTarget, true);
    
    intermediateSlot = cbvAlloc.alloc();
    imguiSlot = cbvAlloc.alloc();

    // create backbuffers
    createBackBuffers();

    // create our intermediate target
    createIntermediateTarget();
}

enum UpdateAttrib : unsigned {
    eUpdateNone = 0,
    eUpdateBackBuffers = (1 << 0),
    eUpdateResolution = (1 << 1)
};

void RenderGraph::update(const Create& create) {
    ASSERT(create.backBuffers == info.backBuffers); // cant change that yet
    ASSERT(create.window == info.window);

    unsigned attribs = eUpdateNone;

    if (create.backBuffers != info.backBuffers) {
        attribs |= eUpdateBackBuffers;
        for (size_t i = 0; i < info.backBuffers; ++i) {
            backBuffers[i].release();
        }
    }

    if (create.resolution != info.resolution) {
        attribs |= eUpdateResolution;
    }

    info = create;

    if (attribs & eUpdateBackBuffers) {
        swapchain.resize(windowSize().as<size_t>(), info.backBuffers);

        createBackBuffers();
        backBuffers = new rhi::Buffer[info.backBuffers];
        for (size_t i = 0; i < info.backBuffers; ++i) {
            backBuffers[i] = swapchain.getBuffer(i);
            device.createRenderTargetView(backBuffers[i], rtvHeap.cpuHandle(i));
        }
    }

    if (attribs & eUpdateResolution) {
        intermediateTarget.release();
        createIntermediateTarget();
    }
}

void RenderGraph::createBackBuffers() {
    currentFrame = swapchain.currentBackBuffer();
    backBuffers = new rhi::Buffer[info.backBuffers];
    for (size_t i = 0; i < info.backBuffers; ++i) {
        backBuffers[i] = swapchain.getBuffer(i);
        allocators[i] = device.newAllocator(rhi::CommandList::Type::eDirect);
        
        device.createRenderTargetView(backBuffers[i], rtvHeap.cpuHandle(getRtvSlot(i)));
    }
}

void RenderGraph::createIntermediateTarget() {
    const auto intermediateTargetDesc = rhi::newTextureDesc(info.resolution, rhi::Buffer::eRenderTarget);
    intermediateTarget = device.newTexture(intermediateTargetDesc, rhi::DescriptorSet::Visibility::eDeviceOnly);

    device.createRenderTargetView(intermediateTarget, rtvHeap.cpuHandle(intermediateRtvSlot()));
    device.createTextureBufferView(intermediateTarget, cbvHeap.cpuHandle(intermediateSlot));
}
