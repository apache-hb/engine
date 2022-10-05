#include "engine/render/render.h"
#include <array>

using namespace engine;
using namespace engine::render;

#define DX_NAME(it) it->SetName(L"" #it)

namespace {
    namespace PostSlots {
        enum Slot : unsigned {
            eImgui, // imgui const buffer
            eFrame, // intermediate render target
            eTotal
        };
    }

    using BufferState = rhi::Buffer::State;
    constexpr math::float4 kLetterBox = { 0.f, 0.f, 0.f, 1.f };

    constexpr Vertex kScreenQuad[] = {
        { { -1.0f, -1.0f, 0.0f }, { 0.0f, 1.0f } }, // bottom left
        { { -1.0f, 1.0f, 0.0f }, { 0.0f, 0.0f } },  // top left
        { { 1.0f, -1.0f, 0.0f }, { 1.0f, 1.0f } },  // bottom right
        { { 1.0f, 1.0f, 0.0f }, { 1.0f, 0.0f } } // top right
    };

    constexpr uint32_t kScreenQuadIndices[] = {
        0, 1, 2,
        1, 3, 2
    };
}

std::vector<std::byte> render::loadShader(std::string_view path) {
    Io *io = Io::open(path, Io::eRead);
    return io->read<std::byte>();
}

Context::Context(Create &&info) : info(info), device(rhi::getDevice(info.channel)) {
    updateViewports();
    create();
}

void Context::updateViewports() {
    auto [windowWidth, windowHeight] = info.window->size();
    auto resolution = info.scene.resolution;

    auto widthRatio = float(resolution.width) / windowWidth;
    auto heightRatio = float(resolution.height) / windowHeight;

    float x = 1.f;
    float y = 1.f;

    if (widthRatio < heightRatio) {
        x = widthRatio / heightRatio;
    } else {
        y = heightRatio / widthRatio;
    }

    postView.viewport.TopLeftX = windowWidth * (1.f - x) / 2.f;
    postView.viewport.TopLeftY = windowHeight * (1.f - y) / 2.f;
    postView.viewport.Width = x * windowWidth;
    postView.viewport.Height = y * windowHeight;

    postView.scissor.left = LONG(postView.viewport.TopLeftX);
    postView.scissor.right = LONG(postView.viewport.TopLeftX + postView.viewport.Width);
    postView.scissor.top = LONG(postView.viewport.TopLeftY);
    postView.scissor.bottom = LONG(postView.viewport.TopLeftY + postView.viewport.Height);
}

void Context::create() {
    // create our swapchain and backbuffer resources
    directQueue = device.newQueue(rhi::CommandList::Type::eDirect);
    DX_NAME(directQueue);

    swapchain = directQueue.newSwapChain(info.window, kFrameCount);
    frameIndex = swapchain.currentBackBuffer();

    renderTargetSet = device.newDescriptorSet(kRenderTargets, rhi::DescriptorSet::Type::eRenderTarget, false);

    postDataHeap = device.newDescriptorSet(PostSlots::eTotal, rhi::DescriptorSet::Type::eConstBuffer, true);

    DX_NAME(postDataHeap);

    for (size_t i = 0; i < kFrameCount; i++) {
        rhi::Buffer target = swapchain.getBuffer(i);
        device.createRenderTargetView(target, renderTargetSet.cpuHandle(i));

        // create per frame data
        renderTargets[i] = std::move(target);
        postAllocators[i] = device.newAllocator(rhi::CommandList::Type::eDirect);
    }

    // create our intermediate render target
    auto result = device.newTexture(info.scene.resolution, rhi::DescriptorSet::Visibility::eDeviceOnly, BufferState::eRenderTarget, kClearColour);
    intermediateTarget = std::move(result.buffer);

    device.createRenderTargetView(intermediateTarget, renderTargetSet.cpuHandle(kFrameCount));
    device.createTextureBufferView(intermediateTarget, postDataHeap.cpuHandle(PostSlots::eFrame));

    DX_NAME(intermediateTarget);

    postCommands = device.newCommandList(postAllocators[frameIndex], rhi::CommandList::Type::eDirect);

    DX_NAME(postCommands);

    // create copy queue resources
    copyAllocator = device.newAllocator(rhi::CommandList::Type::eCopy);
    copyQueue = device.newQueue(rhi::CommandList::Type::eCopy);
    DX_NAME(copyQueue);

    copyCommands = device.newCommandList(copyAllocator, rhi::CommandList::Type::eCopy);

    fence = device.newFence();

    auto input = std::to_array<rhi::InputElement>({
        { "POSITION", rhi::Format::float32x3 },
        { "TEXCOORD", rhi::Format::float32x2 }
    });

    auto postPs = loadShader("resources/shaders/post-shader.ps.pso");
    auto postVs = loadShader("resources/shaders/post-shader.vs.pso");

    auto samplers = std::to_array<rhi::Sampler>({
        { rhi::ShaderVisibility::ePixel }
    });

    auto postRanges = std::to_array<rhi::BindingRange>({
        { 0, rhi::Object::eTexture, rhi::BindingMutability::eStaticAtExecute }
    });

    auto postBindings = std::to_array<rhi::BindingTable>({
        rhi::BindingTable { 
            .visibility = rhi::ShaderVisibility::ePixel,
            .ranges = postRanges
        }
    });

    auto postPipeline = device.newPipelineState({
        .samplers = samplers,
        .tables = postBindings,
        .input = input,
        .ps = postPs,
        .vs = postVs
    });

    DX_NAME(postPipeline.pipeline);
    DX_NAME(postPipeline.signature);

    // upload our data to the gpu

    beginCopy();

    // upload fullscreen quad
    auto postVertexBuffer = uploadData(kScreenQuad, sizeof(kScreenQuad));
    auto postIndexBuffer = uploadData(kScreenQuadIndices, sizeof(kScreenQuadIndices));

    DX_NAME(postVertexBuffer);
    DX_NAME(postIndexBuffer);

    rhi::VertexBufferView vertexBufferView {
        postVertexBuffer.gpuAddress(),
        std::size(kScreenQuad),
        sizeof(Vertex)
    };

    rhi::IndexBufferView indexBufferView {
        postIndexBuffer.gpuAddress(),
        std::size(kScreenQuadIndices),
        rhi::Format::uint32
    };

    screenQuad = Mesh {
        .pso = std::move(postPipeline),

        .vertexBuffer = std::move(postVertexBuffer),
        .indexBuffer = std::move(postIndexBuffer),

        .vertexBufferView = vertexBufferView,
        .indexBufferView = indexBufferView
    };

    ID3D12CommandList *sceneCommands = info.scene.attach(this);

    size_t signal = endCopy();

    ID3D12CommandList* copyCommandList[] = { copyCommands.get() };
    copyQueue.execute(copyCommandList);
    waitOnQueue(copyQueue, signal);


    ID3D12CommandList* directCommandList[] = { sceneCommands, postCommands.get() };
    directQueue.execute(directCommandList);
    waitForFrame();


    pendingCopies.resize(0);
    device.imguiInit(kFrameCount, postDataHeap, postDataHeap.cpuHandle(PostSlots::eImgui), postDataHeap.gpuHandle(PostSlots::eImgui));
}

void Context::begin() {
    device.imguiNewFrame();

    beginPost();
}

void Context::end() {
    endPost();

    ID3D12CommandList *sceneCommands = info.scene.populate(this);

    ID3D12CommandList* commands[] = { sceneCommands, postCommands.get() };
    directQueue.execute(commands);
    swapchain.present();

    waitForFrame();
}

// draw the intermediate render target to the screen
void Context::beginPost() {
    postCommands.beginRecording(postAllocators[frameIndex]);
    postCommands.setPipeline(screenQuad.pso);

    auto kDescriptors = std::to_array({ postDataHeap.get() });
    postCommands.bindDescriptors(kDescriptors);

    // transition into rendering to the intermediate
    postCommands.transition(renderTargets[frameIndex], BufferState::ePresent, BufferState::eRenderTarget);
    postCommands.transition(intermediateTarget, BufferState::eRenderTarget, BufferState::ePixelShaderResource);

    postCommands.bindTable(0, postDataHeap.gpuHandle(PostSlots::eFrame));
    postCommands.setViewAndScissor(postView);

    postCommands.setRenderTarget(renderTargetSet.cpuHandle(frameIndex), kLetterBox);
    postCommands.drawMesh(screenQuad.indexBufferView, screenQuad.vertexBufferView);
}

void Context::endPost() {
    postCommands.imguiRender();

    // now switch back to rendering from intermediate to target
    postCommands.transition(intermediateTarget, BufferState::ePixelShaderResource, BufferState::eRenderTarget);
    postCommands.transition(renderTargets[frameIndex], BufferState::eRenderTarget, BufferState::ePresent);

    postCommands.endRecording();
}

void Context::waitForFrame() {
    waitOnQueue(directQueue, fenceValue++);

    frameIndex = swapchain.currentBackBuffer();
}

void Context::waitOnQueue(rhi::CommandQueue &queue, size_t value) {
    queue.signal(fence, value);
    fence.waitUntil(value);
}

rhi::Buffer Context::uploadTexture(rhi::CommandList &commands, rhi::TextureSize size, std::span<std::byte> data) {
    auto result = device.newTexture(size, rhi::DescriptorSet::Visibility::eDeviceOnly, BufferState::eCopyDst);
    auto upload = device.newBuffer(result.uploadSize, rhi::DescriptorSet::Visibility::eHostVisible, BufferState::eUpload);

    commands.transition(result.buffer, BufferState::eCopyDst, BufferState::ePixelShaderResource);
    copyCommands.copyTexture(result.buffer, upload, data.data(), size);

    pendingCopies.push_back(std::move(upload));

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
    copyCommands.beginRecording(copyAllocator);
}

size_t Context::endCopy() {
    copyCommands.endRecording();
    return ++currentCopy;
}
