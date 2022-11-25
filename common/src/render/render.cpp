#include "engine/render/render.h"
#include "imgui/imgui.h"
#include <array>

using namespace simcoe;
using namespace simcoe::render;

#define DX_NAME(it) it.rename("" #it)

namespace {
    constexpr size_t totalRenderTargets(size_t frames) {
        return frames + 1;
    }

    using BufferState = rhi::Buffer::State;
    constexpr math::float4 kLetterBox = { 0.f, 0.f, 0.f, 1.f };

    constexpr assets::Vertex kScreenQuad[] = {
        { .position = { -1.0f, -1.0f, 0.0f }, .uv = { 0.0f, 1.0f } }, // bottom left
        { .position = { -1.0f, 1.0f, 0.0f },  .uv = { 0.0f, 0.0f } },  // top left
        { .position = { 1.0f,  -1.0f, 0.0f }, .uv = { 1.0f, 1.0f } },  // bottom right
        { .position = { 1.0f,  1.0f, 0.0f },  .uv = { 1.0f, 0.0f } } // top right
    };

    constexpr uint32_t kScreenQuadIndices[] = {
        0, 1, 2,
        1, 3, 2
    };

    constexpr auto kInput = std::to_array<rhi::InputElement>({
        { "POSITION", rhi::Format::float32x3, offsetof(assets::Vertex, position) },
        { "TEXCOORD", rhi::Format::float32x2 , offsetof(assets::Vertex, uv) }
    });

    constexpr auto kSamplers = std::to_array<rhi::Sampler>({
        { rhi::ShaderVisibility::ePixel }
    });

    constexpr auto kRanges = std::to_array<rhi::BindingRange>({
        { 0, rhi::Object::eTexture, rhi::BindingMutability::eStaticAtExecute }
    });

    constexpr auto kBindings = std::to_array<rhi::Binding>({
        rhi::bindTable(rhi::ShaderVisibility::ePixel, kRanges)
    });
}

std::vector<std::byte> render::loadShader(std::string_view path) {
    UniquePtr<Io> io { Io::open(path, Io::eRead) };
    return io->read<std::byte>();
}

Context::Context(Create &&info) 
    : window(info.window)
    , scene(info.scene)
    , frames(info.frames)
    , device(rhi::getDevice()) 
    , cbvAlloc(kHeapSize)
{
    updateViewports(window->size().as<size_t>(), scene->resolution);
    create();
    createScene();
}

Context::~Context() {
    device.imguiShutdown();
}

void Context::updateViewports(rhi::TextureSize post, rhi::TextureSize scene) {
    resolution = post;

    auto [width, height] = post;

    auto widthRatio = float(scene.width) / width;
    auto heightRatio = float(scene.height) / height;

    float x = 1.f;
    float y = 1.f;

    if (widthRatio < heightRatio) {
        x = widthRatio / heightRatio;
    } else {
        y = heightRatio / widthRatio;
    }

    postView.viewport.TopLeftX = width * (1.f - x) / 2.f;
    postView.viewport.TopLeftY = height * (1.f - y) / 2.f;
    postView.viewport.Width = x * width;
    postView.viewport.Height = y * height;

    postView.scissor.left = LONG(postView.viewport.TopLeftX);
    postView.scissor.right = LONG(postView.viewport.TopLeftX + postView.viewport.Width);
    postView.scissor.top = LONG(postView.viewport.TopLeftY);
    postView.scissor.bottom = LONG(postView.viewport.TopLeftY + postView.viewport.Height);
}

void Context::create() {
    // create our swapchain and backbuffer resources
    directQueue = device.newQueue(rhi::CommandList::Type::eDirect);
    DX_NAME(directQueue);

    swapchain = directQueue.newSwapChain(window, frames);
    frameIndex = swapchain.currentBackBuffer();

    renderTargetSet = device.newDescriptorSet(totalRenderTargets(frames), rhi::DescriptorSet::Type::eRenderTarget, false);

    cbvHeap = device.newDescriptorSet(kHeapSize, rhi::DescriptorSet::Type::eConstBuffer, true);

    cbvHeap.rename("cbv-heap");

    intermediateHeapOffset = cbvAlloc.alloc(DescriptorSlot::eIntermediate);
    imguiHeapOffset = cbvAlloc.alloc(DescriptorSlot::eImGui);

    DX_NAME(cbvHeap);

    // TODO: iterators again
    postAllocators = {frames};
    copyAllocators = {frames};
    for (size_t i = 0; i < frames; i++) {
        // create per frame data
        postAllocators[i] = device.newAllocator(rhi::CommandList::Type::eDirect);
        copyAllocators[i] = device.newAllocator(rhi::CommandList::Type::eCopy);
    }

    createRenderTargets();

    // create our intermediate render target
    auto result = device.newTexture(scene->resolution, rhi::DescriptorSet::Visibility::eDeviceOnly, BufferState::eRenderTarget, kClearColour);
    intermediateTarget = std::move(result.buffer);

    device.createRenderTargetView(intermediateTarget, renderTargetSet.cpuHandle(frames));
    device.createTextureBufferView(intermediateTarget, cbvHeap.cpuHandle(intermediateHeapOffset));

    DX_NAME(intermediateTarget);

    postCommands = device.newCommandList(postAllocators[frameIndex], rhi::CommandList::Type::eDirect);

    DX_NAME(postCommands);

    // create copy queue resources
    copyQueue = device.newQueue(rhi::CommandList::Type::eCopy);
    DX_NAME(copyQueue);

    copyCommands = device.newCommandList(copyAllocators[frameIndex], rhi::CommandList::Type::eCopy);

    copyFence = device.newFence();
    directFence = device.newFence();

    auto postPs = loadShader("resources/shaders/post-shader.ps.pso");
    auto postVs = loadShader("resources/shaders/post-shader.vs.pso");

    pso = device.newPipelineState({
        .samplers = kSamplers,
        .bindings = kBindings,
        .input = kInput,
        .ps = postPs,
        .vs = postVs
    });

    DX_NAME(pso.pipeline);
    DX_NAME(pso.signature);

    // upload our data to the gpu

    beginCopy();

    // upload fullscreen quad
    vertexBuffer = uploadData(kScreenQuad, sizeof(kScreenQuad));
    indexBuffer = uploadData(kScreenQuadIndices, sizeof(kScreenQuadIndices));

    DX_NAME(vertexBuffer);
    DX_NAME(indexBuffer);

    vertexBufferView = rhi::newVertexBufferView(vertexBuffer.gpuAddress(), std::size(kScreenQuad), sizeof(assets::Vertex));

    indexBufferView = {
        indexBuffer.gpuAddress(),
        std::size(kScreenQuadIndices),
        rhi::Format::uint32
    };

    size_t signal = endCopy();

    executeCopy(signal);

    // release copy resources that are no longer needed
    device.imguiInit(frames, cbvHeap, imguiHeapOffset);
}

void Context::createScene() {
    beginCopy();

    ID3D12CommandList *sceneCommands = scene->attach(this);

    size_t signal = endCopy();

    executeCopy(signal);

    ID3D12CommandList* directCommandList[] = { sceneCommands };
    directQueue.execute(directCommandList);
    waitForFrame();
}

void Context::begin() {
    device.imguiFrame();

    beginCopy();
    beginPost();
}

void Context::end() {
    ID3D12CommandList *sceneCommands = scene->populate();

    endPost();
    size_t signal = endCopy();

    ID3D12CommandList* commands[] = { sceneCommands, postCommands.get() };
    directQueue.execute(commands);
    swapchain.present();

    waitForFrame();

    executeCopy(signal);
}

void Context::resizeScene(rhi::TextureSize) {
    
}

void Context::resizeOutput(rhi::TextureSize size) {
    // release all back buffers to make sure we can recreate them
    // TODO: we should add range iterators to unique ptr arrays
    for (size_t i = 0; i < frames; i++) {
        renderTargets[i].release();
    }

    updateViewports(size, scene->resolution);
    swapchain.resize(size, frames);
    frameIndex = swapchain.currentBackBuffer();

    createRenderTargets();
}

void Context::createRenderTargets() {
    renderTargets = {frames};
    
    for (size_t i = 0; i < frames; i++) {
        renderTargets[i] = swapchain.getBuffer(i);
        device.createRenderTargetView(renderTargets[i], renderTargetSet.cpuHandle(i));
    }
}

// draw the intermediate render target to the screen
void Context::beginPost() {
    auto kDescriptors = std::to_array({ cbvHeap.get() });
    auto kVerts = std::to_array({ vertexBufferView });
    auto kTransitions = std::to_array({
        rhi::newStateTransition(renderTargets[frameIndex], BufferState::ePresent, BufferState::eRenderTarget),
        rhi::newStateTransition(intermediateTarget, BufferState::eRenderTarget, BufferState::ePixelShaderResource)
    });

    // TODO: race condition?
    postCommands.beginRecording(postAllocators[frameIndex]);
    if (!pendingTransitions.empty()) {
        postCommands.transition(pendingTransitions);
        pendingTransitions.clear();
    }

    postCommands.setPipeline(pso);

    postCommands.bindDescriptors(kDescriptors);

    // transition into rendering to the intermediate
    postCommands.transition(kTransitions);

    postCommands.bindTable(0, cbvHeap.gpuHandle(intermediateHeapOffset));
    postCommands.setViewAndScissor(postView);

    postCommands.setVertexBuffers(kVerts);
    postCommands.setRenderTarget(renderTargetSet.cpuHandle(frameIndex), rhi::CpuHandle::Invalid, kLetterBox);
    postCommands.drawMesh(indexBufferView);
}

void Context::endPost() {
    auto kTransitions = std::to_array({
        rhi::newStateTransition(renderTargets[frameIndex], BufferState::eRenderTarget, BufferState::ePresent),
        rhi::newStateTransition(intermediateTarget, BufferState::ePixelShaderResource, BufferState::eRenderTarget)
    });

    postCommands.imguiFrame();

    // now switch back to rendering from intermediate to target
    postCommands.transition(kTransitions);

    postCommands.endRecording();
}

void Context::waitForFrame() {
    waitOnQueue(directQueue, fenceValue++, directFence);

    frameIndex = swapchain.currentBackBuffer();
}

void Context::waitOnQueue(rhi::CommandQueue &queue, size_t value, rhi::Fence& fence) {
    queue.signal(fence, value);
    fence.wait(value);
}

rhi::Buffer Context::uploadTexture(rhi::CpuHandle handle, rhi::TextureSize size) {
    auto result = device.newTexture(size, rhi::DescriptorSet::Visibility::eDeviceOnly, BufferState::eCopyDst);
    auto upload = device.newBuffer(result.uploadSize, rhi::DescriptorSet::Visibility::eHostVisible, BufferState::eUpload);

    pendingTransitions.push_back(rhi::newStateTransition(result.buffer, BufferState::eCopyDst, BufferState::ePixelShaderResource));

    copyCommands.copyTexture(result.buffer, upload);

    pendingCopies.push_back(std::move(upload));

    device.createTextureBufferView(result.buffer, handle);

    return std::move(result.buffer);
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
    copyCommands.beginRecording(copyAllocators[frameIndex]);
}

size_t Context::endCopy() {
    copyCommands.endRecording();
    return ++currentCopy;
}

void Context::executeCopy(size_t signal) {
    ID3D12CommandList* commands[] = { copyCommands.get() };
    copyQueue.execute(commands);
    waitOnQueue(copyQueue, signal, copyFence);

    // release copy resources that are no longer needed
    pendingCopies.resize(0);
}
