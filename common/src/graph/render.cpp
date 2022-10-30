#include "engine/graph/render.h"
#include "engine/rhi/rhi.h"

using namespace simcoe;
using namespace simcoe::graph;

RenderGraph::RenderGraph(const Create& create) : info(create) { 
    // create all device resources
    auto *setup = newPass<SetupPass>("setup");
    
    UNUSED auto *commands = newPass<CommandPass>("commands");
    
    // clear backbuffer
    auto *clear = newPass<ClearPass>("clear", math::float4::of(0));

    // present backbuffer
    auto *present = newPass<PresentPass>("present");

    bind(
        { setup, "swapchain" },
        { present, "swapchain" }
    );

    bind(
        { setup, "backbuffer" },
        { clear, "buffer" }
    );

    bind(
        { clear, "buffer" },
        { present, "backbuffer" }
    );
}

void RenderGraph::bind(RenderPassSlot source, RenderPassSlot target) {
    bindings.push_back({ source, target });
}

struct ResourceBind {
    Input *source;
    Output *target;
};

struct RenderPassTree {
    using BindVec = std::vector<ResourceBind>;
    using PassTreeVec = std::vector<RenderPassTree>;

    RenderPassTree(RenderPass *pass, BindVec binds, PassTreeVec trees) 
        : pass(pass)
        , binds(binds)
        , children(trees) 
    { }

    RenderPass *pass;
    BindVec binds;
    PassTreeVec children;

    bool executed = false;

    void execute() {
        if (executed) { return; }
        executed = true;

        for (auto &child : children) {
            child.execute();
        }

        // TODO: transitions?
        std::vector<rhi::StateTransition> transitions;
        for (auto &[source, target] : binds) {
            ASSERT(source->get() == target->get());

            auto before = source->getState();
            auto after = target->getState();
            if (before == after) { continue; }

            auto *resource = source->get();

            resource->transition(before, after, transitions);
        }
    }

    void clear() {
        executed = false;
        for (auto &child : children) {
            child.clear();
        }
    }
};

struct RenderPassInfo {
    std::vector<RenderPass*> passes;
    std::vector<ResourceBind> binds;

    void add(ResourceBind& bind, RenderPass *child) {
        binds.push_back(bind);
        passes.push_back(child);
    }
};

struct RenderTreeBuilder {
    // map of passes to the inputs they need
    std::unordered_map<RenderPass*, RenderPassInfo> passes;

    RenderPassTree build(RenderPass *root) {
        std::vector<RenderPassTree> children;
        auto [info, binds] = passes[root];
        for (auto pass : info) {
            children.push_back(build(pass));
        }
        return RenderPassTree(root, binds, children);
    }
};

void RenderGraph::render() {
    RenderTreeBuilder builder;
    RenderPass *root = nullptr;

    for (auto& [source, target] : bindings) {
        auto [sourcePass, sourceSlot] = source;
        auto [targetPass, targetSlot] = target;

        ResourceBind bind = {
            .source = sourcePass->getInput(sourceSlot),
            .target = targetPass->getOutput(targetSlot)
        };

        builder.passes[targetPass].add(bind, sourcePass);

        if (strcmp(sourcePass->getName(), info.root) == 0) {
            root = sourcePass;
        }
    }

    ASSERT(root != nullptr);

    auto tree = builder.build(root);

    tree.execute();
}

#if 0
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

    // caclulate viewport and scissor rects
    updateViewport(windowSize(), info.resolution);

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
            allocators[i].release();
        }
    }

    if (create.resolution != info.resolution) {
        attribs |= eUpdateResolution;
    }

    info = create;

    if (attribs & eUpdateBackBuffers) {
        swapchain.resize(windowSize(), info.backBuffers);
        createBackBuffers();
    }

    if (attribs & eUpdateResolution) {
        intermediateTarget.release();
        updateViewport(windowSize(), info.resolution);
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

void RenderGraph::updateViewport(rhi::TextureSize display, rhi::TextureSize resolution) {
    auto [width, height] = display;

    auto widthRatio = float(resolution.width) / width;
    auto heightRatio = float(resolution.height) / height;

    float x = 1.f;
    float y = 1.f;

    if (widthRatio < heightRatio) {
        x = widthRatio / heightRatio;
    } else {
        y = heightRatio / widthRatio;
    }

    view.viewport.TopLeftX = width * (1.f - x) / 2.f;
    view.viewport.TopLeftY = height * (1.f - y) / 2.f;
    view.viewport.Width = x * width;
    view.viewport.Height = y * height;

    view.scissor.left = LONG(view.viewport.TopLeftX);
    view.scissor.right = LONG(view.viewport.TopLeftX + view.viewport.Width);
    view.scissor.top = LONG(view.viewport.TopLeftY);
    view.scissor.bottom = LONG(view.viewport.TopLeftY + view.viewport.Height);
}
#endif