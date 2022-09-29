#include "engine/render/render.h"
#include <array>

#include "stb_image.h"

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

    namespace SceneSlots {
        enum Slot : unsigned {
            eCamera, // camera matrix
            eTexture, // object texture
            eTotal
        };
    }

    using BufferState = rhi::Buffer::State;
    constexpr math::float4 kClearColour = { 0.f, 0.2f, 0.4f, 1.f };
    constexpr math::float4 kLetterBox = { 0.f, 0.f, 0.f, 1.f };

    std::vector<std::byte> loadShader(std::string_view path) {
        Io *io = Io::open(path, Io::eRead);
        return io->read<std::byte>();
    }

    struct Image {
        std::vector<std::byte> data;
        size_t width;
        size_t height;
    };

    Image loadImage(const char *path) {
        int x, y, channels;
        auto *it = reinterpret_cast<std::byte*>(stbi_load(path, &x, &y, &channels, 4));
        
        return Image {
            .data = std::vector(it, it + (x * y * 4)),
            .width = size_t(x),
            .height = size_t(y)
        };
    }

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

Context::Context(Create &&info) : info(info), device(rhi::getDevice()) {
    auto size = info.window->size();
    auto [width, height] = info.resolution;

    postViewport = { float(size.width), float(size.height) };

    sceneViewport = { float(width), float(height) };
    aspectRatio = size.aspectRatio<float>();

    create();
}

void Context::create() {
    // create our swapchain and backbuffer resources
    directQueue = device.newQueue(rhi::CommandList::Type::eDirect);
    DX_NAME(directQueue);

    swapchain = directQueue.newSwapChain(info.window, kFrameCount);
    frameIndex = swapchain.currentBackBuffer();

    renderTargetSet = device.newDescriptorSet(kRenderTargets, rhi::DescriptorSet::Type::eRenderTarget, false);
    
    postDataHeap = device.newDescriptorSet(PostSlots::eTotal, rhi::DescriptorSet::Type::eConstBuffer, true);
    sceneDataHeap = device.newDescriptorSet(SceneSlots::eTotal, rhi::DescriptorSet::Type::eConstBuffer, true);

    DX_NAME(postDataHeap);
    DX_NAME(sceneDataHeap);

    for (size_t i = 0; i < kFrameCount; i++) {
        rhi::Buffer target = swapchain.getBuffer(i);
        device.createRenderTargetView(target, renderTargetSet.cpuHandle(i));

        // create per frame data
        renderTargets[i] = std::move(target);
        sceneAllocators[i] = device.newAllocator(rhi::CommandList::Type::eDirect);
        postAllocators[i] = device.newAllocator(rhi::CommandList::Type::eDirect);
    }

    // create our intermediate render target
    auto result = device.newTexture(info.resolution, rhi::DescriptorSet::Visibility::eDeviceOnly, BufferState::eRenderTarget, kClearColour);
    intermediateTarget = std::move(result.buffer);

    device.createRenderTargetView(intermediateTarget, renderTargetSet.cpuHandle(kFrameCount));
    device.createTextureBufferView(intermediateTarget, postDataHeap.cpuHandle(PostSlots::eFrame));

    DX_NAME(intermediateTarget);

    sceneCommands = device.newCommandList(sceneAllocators[frameIndex], rhi::CommandList::Type::eDirect);
    postCommands = device.newCommandList(postAllocators[frameIndex], rhi::CommandList::Type::eDirect);

    DX_NAME(sceneCommands);
    DX_NAME(postCommands);

    // create copy queue resources
    copyAllocator = device.newAllocator(rhi::CommandList::Type::eCopy);
    copyQueue = device.newQueue(rhi::CommandList::Type::eCopy);
    DX_NAME(copyQueue);

    copyCommands = device.newCommandList(copyAllocator, rhi::CommandList::Type::eCopy);

    fence = device.newFence();

    // const buffer binding data
    constBuffer = device.newBuffer(sizeof(ConstBuffer), rhi::DescriptorSet::Visibility::eHostVisible, BufferState::eUpload);
    device.createConstBufferView(constBuffer, sizeof(ConstBuffer), sceneDataHeap.cpuHandle(SceneSlots::eCamera));
    constBufferPtr = constBuffer.map();
    memcpy(constBufferPtr, &constBufferData, sizeof(ConstBuffer));

    auto input = std::to_array<rhi::InputElement>({
        { "POSITION", rhi::Format::float32x3 },
        { "TEXCOORD", rhi::Format::float32x2 }
    });

    auto postPs = loadShader("resources/shaders/post-shader.ps.pso");
    auto postVs = loadShader("resources/shaders/post-shader.vs.pso");
    
    auto scenePs = loadShader("resources/shaders/scene-shader.ps.pso");
    auto sceneVs = loadShader("resources/shaders/scene-shader.vs.pso");
    
    auto samplers = std::to_array<rhi::Sampler>({
        { rhi::ShaderVisibility::ePixel }
    });

    auto postBindings = std::to_array<rhi::Binding>({
        rhi::Binding { 0, rhi::Object::eTexture, rhi::BindingMutability::eStaticAtExecute } // register(t0, space0)
    });

    auto sceneBindings = std::to_array<rhi::Binding>({
        rhi::Binding { 0, rhi::Object::eConstBuffer, rhi::BindingMutability::eStaticAtExecute }, // register(b0, space0)
        rhi::Binding { 0, rhi::Object::eTexture, rhi::BindingMutability::eAlwaysStatic } // register(t0, space0)
    });

    auto postPipeline = device.newPipelineState({
        .samplers = samplers,
        .bindings = postBindings,
        .input = input,
        .ps = postPs,
        .vs = postVs
    });

    auto scenePipeline = device.newPipelineState({
        .samplers = samplers,
        .bindings = sceneBindings,
        .input = input,
        .ps = scenePs,
        .vs = sceneVs
    });

    DX_NAME(postPipeline.pipeline);
    DX_NAME(postPipeline.signature);
    
    DX_NAME(scenePipeline.pipeline);
    DX_NAME(scenePipeline.signature);
    

    // upload our data to the gpu

    auto image = loadImage("C:\\Users\\ehb56\\Downloads\\uv_grid.jpg");

    sceneCommands.beginRecording(sceneAllocators[frameIndex]);

    size_t signal = recordCopy([&] {
        // upload texture
        textureBuffer = uploadTexture({ image.width, image.height }, image.data);
        DX_NAME(textureBuffer);

        device.createTextureBufferView(textureBuffer, sceneDataHeap.cpuHandle(SceneSlots::eTexture));

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

        auto sceneVertexBuffer = uploadData(kScreenQuad, sizeof(kScreenQuad));
        auto sceneIndexBuffer = uploadData(kScreenQuadIndices, sizeof(kScreenQuadIndices));

        DX_NAME(sceneVertexBuffer);
        DX_NAME(sceneIndexBuffer);

        rhi::VertexBufferView sceneVertexBufferView {
            sceneVertexBuffer.gpuAddress(), 
            std::size(kScreenQuad), 
            sizeof(Vertex)
        };
        
        rhi::IndexBufferView sceneIndexBufferView {
            sceneIndexBuffer.gpuAddress(), 
            std::size(kScreenQuadIndices), 
            rhi::Format::uint32
        };
    
        sceneObject = Mesh {
            .pso = std::move(scenePipeline),
            
            .vertexBuffer = std::move(sceneVertexBuffer),
            .indexBuffer = std::move(sceneIndexBuffer),

            .vertexBufferView = sceneVertexBufferView,
            .indexBufferView = sceneIndexBufferView
        };
    });
    
    sceneCommands.endRecording();


    ID3D12CommandList* copyCommandList[] = { copyCommands.get() };
    copyQueue.execute(copyCommandList);
    waitOnQueue(copyQueue, signal);


    ID3D12CommandList* directCommandList[] = { sceneCommands.get(), postCommands.get() };
    directQueue.execute(directCommandList);
    waitForFrame();


    pendingCopies.resize(0);
    device.imguiInit(kFrameCount, postDataHeap, postDataHeap.cpuHandle(PostSlots::eImgui), postDataHeap.gpuHandle(PostSlots::eImgui));
}

void Context::begin(Camera *camera) {
    constBufferData.mvp = camera->mvp(float4x4::identity(), aspectRatio);
    memcpy(constBufferPtr, &constBufferData, sizeof(ConstBuffer));

    device.imguiNewFrame();

    beginScene();
    beginPost();
}

void Context::end() {
    endScene();
    endPost();

    ID3D12CommandList* commands[] = { sceneCommands.get(), postCommands.get() };
    directQueue.execute(commands);
    swapchain.present();

    waitForFrame();
}

void Context::beginScene() {
    sceneCommands.beginRecording(sceneAllocators[frameIndex]);
    sceneCommands.setPipeline(sceneObject.pso);

    auto kDescriptors = std::to_array({ sceneDataHeap.get() });
    sceneCommands.bindDescriptors(kDescriptors);

    sceneCommands.bindTable(0, sceneDataHeap.gpuHandle(0));

    sceneCommands.setViewport(sceneViewport);
    sceneCommands.setRenderTarget(renderTargetSet.cpuHandle(kFrameCount), kClearColour); // render to the intermediate framebuffer

    sceneCommands.drawMesh(sceneObject.indexBufferView, sceneObject.vertexBufferView);
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
    postCommands.setViewport(postViewport); 

    postCommands.setRenderTarget(renderTargetSet.cpuHandle(frameIndex), kLetterBox);
    postCommands.drawMesh(screenQuad.indexBufferView, screenQuad.vertexBufferView);
}

void Context::endScene() {
    sceneCommands.endRecording();
}

void Context::endPost() {
    postCommands.imguiRender();

    // now switch back to rendering from intermediate to target
    postCommands.transition(renderTargets[frameIndex], BufferState::eRenderTarget, BufferState::ePresent);
    postCommands.transition(intermediateTarget, BufferState::ePixelShaderResource, BufferState::eRenderTarget);

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

rhi::Buffer Context::uploadTexture(rhi::TextureSize size, std::span<std::byte> data) {
    auto result = device.newTexture(size, rhi::DescriptorSet::Visibility::eDeviceOnly, BufferState::eCopyDst);
    auto upload = device.newBuffer(result.uploadSize, rhi::DescriptorSet::Visibility::eHostVisible, BufferState::eUpload);
    
    sceneCommands.transition(result.buffer, BufferState::eCopyDst, BufferState::ePixelShaderResource);
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
