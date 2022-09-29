#include "engine/render/render.h"
#include <array>

using namespace engine;
using namespace engine::render;

namespace {
    namespace Slots {
        enum Slot : unsigned {
            eCamera,
            eImgui,
            eTexture,
            eTotal
        };
    }

    using BufferState = rhi::Buffer::State;
    constexpr math::float4 kClearColour = { 0.f, 0.2f, 0.4f, 1.f };

    std::vector<std::byte> loadShader(std::string_view path) {
        Io *io = Io::open(path, Io::eRead);
        return io->read<std::byte>();
    }

#if 0
    constexpr auto kScreenQuad = std::to_array<Vertex>({
        Vertex { { -1.f, 1.f, 0.f }, { 0.f, 1.f, 0.f, 0.f } }, // top left
        Vertex { { -1.f, -1.f, 0.f }, { 0.f, 0.f, 0.f, 0.f } }, // bottom left
        Vertex { { 1.f, -1.f, 0.f }, { 1.f, 0.f, 0.f, 0.f } }, // bottom right
        Vertex { { 1.f, 1.f, 0.f }, { 1.f, 1.f, 0.f, 0.f } } // top right
    });

    constexpr auto kScreenQuadIndices = std::to_array<uint32_t>({
        0, 1, 2,
        2, 1, 3
    });
#endif
}

Context::Context(Create &&info) : info(info), device(rhi::getDevice()) {
    auto size = info.window->size();
    viewport = { float(size.width), float(size.height) };
    aspectRatio = size.aspectRatio<float>();

    create();
}

void Context::create() {
    // create our swapchain and backbuffer resources
    directQueue = device.newQueue(rhi::CommandList::Type::eDirect);
    swapchain = directQueue.newSwapChain(info.window, kFrameCount);
    frameIndex = swapchain.currentBackBuffer();

    renderTargetSet = device.newDescriptorSet(kFrameCount, rhi::DescriptorSet::Type::eRenderTarget, false);
    for (size_t i = 0; i < kFrameCount; i++) {
        rhi::Buffer target = swapchain.getBuffer(i);
        device.createRenderTargetView(target, renderTargetSet.cpuHandle(i));

        renderTargets[i] = std::move(target);
        allocators[i] = device.newAllocator(rhi::CommandList::Type::eDirect);
    }

    directCommands = device.newCommandList(allocators[frameIndex], rhi::CommandList::Type::eDirect);

    // create copy queue resources
    copyAllocator = device.newAllocator(rhi::CommandList::Type::eCopy);
    copyQueue = device.newQueue(rhi::CommandList::Type::eCopy);
    copyCommands = device.newCommandList(copyAllocator, rhi::CommandList::Type::eCopy);

    fence = device.newFence();

    auto input = std::to_array<rhi::InputElement>({
        { "POSITION", rhi::Format::float32x3 },
        { "TEXCOORD", rhi::Format::float32x2 }
    });

    auto ps = loadShader("resources/shaders/post-shader.ps.pso");
    auto vs = loadShader("resources/shaders/post-shader.vs.pso");
    
    auto samplers = std::to_array<rhi::Sampler>({
        { rhi::ShaderVisibility::ePixel }
    });

    constBufferSet = device.newDescriptorSet(Slots::eTotal, rhi::DescriptorSet::Type::eConstBuffer, true);

    device.imguiInit(kFrameCount, constBufferSet, constBufferSet.cpuHandle(Slots::eImgui), constBufferSet.gpuHandle(Slots::eImgui));

    // const buffer binding data

    constBuffer = device.newBuffer(sizeof(ConstBuffer), rhi::DescriptorSet::Visibility::eHostVisible, rhi::Buffer::State::eUpload);
    device.createConstBufferView(constBuffer, sizeof(ConstBuffer), constBufferSet.cpuHandle(Slots::eCamera));
    constBufferPtr = constBuffer.map();
    memcpy(constBufferPtr, &constBufferData, sizeof(ConstBuffer));

    auto bindings = std::to_array<rhi::Binding>({
        rhi::Binding { 0, rhi::Object::eConstBuffer, rhi::BindingMutability::eStaticAtExecute }, // register(b0, space0)
        rhi::Binding { 0, rhi::Object::eTexture, rhi::BindingMutability::eAlwaysStatic } // register(t0, space0)
    });

    pipeline = device.newPipelineState({
        .samplers = samplers,
        .bindings = bindings,
        .input = input,
        .ps = ps,
        .vs = vs
    });

    // upload our data to the gpu

    const Vertex kVerts[] = {
        { { -1.f, -1.f, 0.0f }, { 0.f, 1.f } }, // bottom left
        { { -1.f, 1.f, 0.0f }, { 0.f, 0.f } }, // top left
        { { 1.f, -1.f, 0.f }, { 1.f, 1.f } }, // bottom right
        { { 1.f, 1.f, 0.f }, { 1.f, 0.f } } // top right
    };

    const uint32_t kIndices[] = {
        0, 1, 2,
        2, 1, 3
    };

    size_t signal = recordCopy([&] {
        vertexBuffer = uploadData(kVerts, sizeof(kVerts));
        indexBuffer = uploadData(kIndices, sizeof(kIndices));

        vertexBufferView = rhi::VertexBufferView{vertexBuffer.gpuAddress(), std::size(kVerts), sizeof(Vertex)};
        indexBufferView = rhi::IndexBufferView{indexBuffer.gpuAddress(), std::size(kIndices), rhi::Format::uint32};
    });

    ID3D12CommandList* commands[] = { copyCommands.get() };
    copyQueue.execute(commands);

    waitOnQueue(copyQueue, signal);
    pendingCopies.resize(0);
    // wait for the direct queue to be available

    waitForFrame();
}

void Context::begin(Camera *camera) {
    constBufferData.mvp = camera->mvp(float4x4::identity(), aspectRatio);
    memcpy(constBufferPtr, &constBufferData, sizeof(ConstBuffer));

    const rhi::CpuHandle rtvHandle = renderTargetSet.cpuHandle(frameIndex);

    device.imguiNewFrame();

    directCommands.beginRecording(allocators[frameIndex]);

    directCommands.transition(renderTargets[frameIndex], rhi::Buffer::State::ePresent, rhi::Buffer::State::eRenderTarget);

    directCommands.setViewport(viewport);
    directCommands.setRenderTarget(rtvHandle, kClearColour);

    directCommands.setPipeline(pipeline);

    auto kDescriptors = std::to_array({ constBufferSet.get() });
    directCommands.bindDescriptors(kDescriptors);
    directCommands.bindTable(0, constBufferSet.gpuHandle(0));

    directCommands.drawMesh(indexBufferView, vertexBufferView);
}

void Context::end() {
    directCommands.imguiRender();
    directCommands.transition(renderTargets[frameIndex], BufferState::eRenderTarget, BufferState::ePresent);

    directCommands.endRecording();

    ID3D12CommandList* commands[] = { directCommands.get() };
    directQueue.execute(commands);
    swapchain.present();

    waitForFrame();
}

void Context::waitForFrame() {
    const auto value = fenceValue++;

    waitOnQueue(directQueue, value);

    frameIndex = swapchain.currentBackBuffer();
}

void Context::waitOnQueue(rhi::CommandQueue &queue, size_t value) {
    queue.signal(fence, value);
    fence.waitUntil(value);
}

rhi::Buffer Context::uploadData(const void *ptr, size_t size) {
    rhi::Buffer upload = device.newBuffer(size, rhi::DescriptorSet::Visibility::eHostVisible, BufferState::eUpload);
    rhi::Buffer result = device.newBuffer(size, rhi::DescriptorSet::Visibility::eDeviceOnly, BufferState::eCopyDst);

    upload.write(ptr, size);
    copyCommands.copyBuffer(result, upload, size);

    pendingCopies.emplace_back(std::move(upload));

    return result;
}

void Context::beginCopy() {
    copyCommands.beginRecording(copyAllocator);
}

size_t Context::endCopy() {
    copyCommands.endRecording();
    return currentCopy++;
}
