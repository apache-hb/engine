#include "engine/render/render.h"
#include <array>

using namespace engine;
using namespace engine::render;

namespace {
    using BufferState = rhi::Buffer::State;
    constexpr math::float4 kClearColour = { 0.f, 0.2f, 0.4f, 1.f };

    std::vector<std::byte> loadShader(std::string_view path) {
        Io *io = Io::open(path, Io::eRead);
        return io->read<std::byte>();
    }
}

Context::Context(Create &&info): info(info) {
    auto [width, height] = info.window->size();
    viewport = { float(width), float(height) };

    create();
}

Context::~Context() {
    destroy();
}

void Context::create() {
    device = rhi::getDevice();

    // create our swapchain and backbuffer resources
    directQueue = device->newQueue(rhi::CommandList::eDirect);
    swapchain = directQueue->newSwapChain(info.window, kFrameCount);
    frameIndex = swapchain->currentBackBuffer();

    renderTargetSet = device->newDescriptorSet(kFrameCount, rhi::Object::eRenderTarget, false);
    for (size_t i = 0; i < kFrameCount; i++) {
        rhi::Buffer *target = swapchain->getBuffer(i);
        device->createRenderTargetView(target, renderTargetSet->cpuHandle(i));

        renderTargets[i] = target;
        allocators[i] = device->newAllocator(rhi::CommandList::eDirect);
    }

    directCommands = device->newCommandList(allocators[frameIndex], rhi::CommandList::eDirect);

    // create copy queue resources
    copyAllocator = device->newAllocator(rhi::CommandList::eCopy);
    copyQueue = device->newQueue(rhi::CommandList::eCopy);
    copyCommands = device->newCommandList(copyAllocator, rhi::CommandList::eCopy);

    fence = device->newFence();

    auto input = std::to_array<rhi::InputElement>({
        { "POSITION", rhi::Format::float32x3 },
        { "COLOUR", rhi::Format::float32x4 }
    });

    auto ps = loadShader("resources/shaders/post-shader.ps.pso");
    auto vs = loadShader("resources/shaders/post-shader.vs.pso");
    
    auto samplers = std::to_array<rhi::Sampler>({
        { rhi::ShaderVisibility::ePixel }
    });

    auto bindings = std::to_array<rhi::Binding>({
        rhi::Binding { rhi::Object::eTexture, 0 }
    });

    pipeline = device->newPipelineState({
        .samplers = samplers,
        .bindings = bindings,
        .input = input,
        .ps = ps,
        .vs = vs
    });

    // upload our data to the gpu

    auto aspect = info.window->size().aspectRatio<float>();

    const Vertex kVerts[] = {
        { { -0.25f, -0.25f * aspect, 0.25f }, { 1.f, 0.f, 0.f, 1.f } },
        { { -0.25f, 0.25f * aspect, 0.25f }, { 0.f, 1.f, 0.f, 1.f } },
        { { 0.25f, -0.25f * aspect, 0.f }, { 0.f, 0.f, 1.f, 1.f } },
        { { 0.25f, 0.25f * aspect, 0.f }, { 1.f, 1.f, 0.f, 1.f } }
    };

    const uint32_t kIndices[] = {
        0, 1, 2,
        2, 1, 3
    };

    copyCommands->beginRecording(copyAllocator);

    vertexBuffer = uploadData(kVerts, sizeof(kVerts));
    indexBuffer = uploadData(kIndices, sizeof(kIndices));

    vertexBufferView = rhi::VertexBufferView{vertexBuffer, std::size(kVerts), sizeof(Vertex)};
    indexBufferView = rhi::IndexBufferView{indexBuffer, std::size(kIndices), rhi::Format::uint32};

    copyCommands->endRecording();

    auto commands = std::to_array({ copyCommands });
    copyQueue->execute(commands);

    waitOnQueue(copyQueue, 1);
    for (rhi::Buffer *buffer : pendingCopies) {
        delete buffer;
    }

    // wait for the direct queue to be available

    waitForFrame();
}

void Context::destroy() {
    delete fence;

    delete pipeline;

    delete vertexBuffer;
    delete indexBuffer;

    delete copyAllocator;
    delete copyCommands;
    delete copyQueue;

    delete directCommands;

    for (size_t i = 0; i < kFrameCount; i++) {
        delete renderTargets[i];
        delete allocators[i];
    }
    
    delete renderTargetSet;

    delete swapchain;
    delete directQueue;

    delete device;
}

void Context::begin() {
    const rhi::CpuHandle rtvHandle = renderTargetSet->cpuHandle(frameIndex);

    directCommands->beginRecording(allocators[frameIndex]);

    directCommands->transition(renderTargets[frameIndex], rhi::Buffer::State::ePresent, rhi::Buffer::State::eRenderTarget);

    directCommands->setViewport(viewport);
    directCommands->setRenderTarget(rtvHandle, kClearColour);

    directCommands->setPipeline(pipeline);
    directCommands->drawMesh(indexBufferView, vertexBufferView);
}

void Context::end() {
    directCommands->transition(renderTargets[frameIndex], BufferState::eRenderTarget, BufferState::ePresent);

    directCommands->endRecording();

    auto commands = std::to_array({ directCommands });
    directQueue->execute(commands);
    swapchain->present();

    waitForFrame();
}

void Context::waitForFrame() {
    const auto value = fenceValue++;

    waitOnQueue(directQueue, value);

    frameIndex = swapchain->currentBackBuffer();
}

void Context::waitOnQueue(rhi::CommandQueue *queue, size_t value) {
    queue->signal(fence, value);
    fence->waitUntil(value);
}

rhi::Buffer *Context::uploadData(const void *ptr, size_t size) {
    rhi::Buffer *upload = device->newBuffer(size, BufferState::eUpload);
    rhi::Buffer *result = device->newBuffer(size, BufferState::eCopyDst);

    upload->write(ptr, size);
    copyCommands->copyBuffer(result, upload, size);

    pendingCopies.push_back(upload);

    return result;
}
