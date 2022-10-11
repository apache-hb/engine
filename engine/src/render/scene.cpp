#include "engine/render/scene.h"

#include <array>

using namespace engine;
using namespace engine::render;

namespace {
    constexpr auto inputLayout = std::to_array<rhi::InputElement>({
        { "POSITION", rhi::Format::float32x3 },
        { "TEXCOORD", rhi::Format::float32x2 }
    });

    constexpr auto samplers = std::to_array<rhi::Sampler>({
        { rhi::ShaderVisibility::ePixel }
    });

    constexpr auto sceneVertexRanges = std::to_array<rhi::BindingRange>({
        { 0, rhi::Object::eConstBuffer, rhi::BindingMutability::eStaticAtExecute } // register(b0) is global data
    });

    constexpr auto scenePixelRanges = std::to_array<rhi::BindingRange>({
        { 0, rhi::Object::eTexture, rhi::BindingMutability::eAlwaysStatic, SIZE_MAX } // register(t0...) are all the textures
    });

    constexpr auto sceneBindings = std::to_array<rhi::Binding>({
        rhi::bindTable(rhi::ShaderVisibility::eVertex, sceneVertexRanges),
        rhi::bindConst(rhi::ShaderVisibility::ePixel, 1, 1), // register(b1) is per object data
        rhi::bindTable(rhi::ShaderVisibility::ePixel, scenePixelRanges)
    });

    namespace Slots {
        enum Slot : unsigned {
            eCamera, // camera matrix
            eTextures
        };
    }

    constexpr Vertex kCubeVerts[] = {
        { { -1.0f, -1.0f, 0.0f }, { 0.0f, 1.0f } }, // bottom left
        { { -1.0f, 1.0f, 0.0f }, { 0.0f, 0.0f } },  // top left
        { { 1.0f, -1.0f, 0.0f }, { 1.0f, 1.0f } },  // bottom right
        { { 1.0f, 1.0f, 0.0f }, { 1.0f, 0.0f } } // top right
    };

    constexpr uint32_t kCubeIndices[] = {
        0, 1, 2,
        1, 3, 2
    };
}

BasicScene::BasicScene(Create&& info) : RenderScene(info.resolution), world(info.world), camera(info.camera) {
    auto [width, height] = resolution;
    view = rhi::View(0, 0, float(width), float(height));
    aspectRatio = resolution.aspectRatio<float>();
}

ID3D12CommandList *BasicScene::populate(Context *ctx) {
    commands.beginRecording(allocators[ctx->currentFrame()]);

    camera.update(aspectRatio);

    commands.setPipeline(mesh.pso);

    auto kDescriptors = std::to_array({ heap.get() });
    commands.bindDescriptors(kDescriptors);

    commands.bindTable(0, heap.gpuHandle(Slots::eCamera));
    commands.bindConst(1, 0, 0);
    commands.bindTable(2, heap.gpuHandle(Slots::eTextures));

    commands.setViewAndScissor(view);
    commands.setRenderTarget(ctx->getRenderTarget(), kClearColour); // render to the intermediate framebuffer

    commands.drawMesh(mesh.indexBufferView, mesh.vertexBufferView);

    commands.endRecording();

    return commands.get();
}

ID3D12CommandList *BasicScene::attach(Context *ctx) {
    auto &device = ctx->getDevice();

    for (size_t i = 0; i < kFrameCount; i++) {
        allocators[i] = device.newAllocator(rhi::CommandList::Type::eDirect);
    }

    heap = device.newDescriptorSet(cbvSetSize(), rhi::DescriptorSet::Type::eConstBuffer, true);

    camera.attach(device, heap.cpuHandle(Slots::eCamera));

    commands = device.newCommandList(allocators[ctx->currentFrame()], rhi::CommandList::Type::eDirect);

    auto ps = loadShader("resources/shaders/scene-shader.ps.pso");
    auto vs = loadShader("resources/shaders/scene-shader.vs.pso");

    auto pipeline = device.newPipelineState({
        .samplers = samplers,
        .bindings = sceneBindings,
        .input = inputLayout,
        .ps = ps,
        .vs = vs
    });

    commands.beginRecording(allocators[ctx->currentFrame()]);

    for (size_t i = 0; i < std::size(world->textures); i++) {
        const auto &image = world->textures[i];
        auto it = ctx->uploadTexture(commands, image.size, image.data);
        device.createTextureBufferView(it, heap.cpuHandle(Slots::eTextures + i));
        textures.push_back(std::move(it));
    }

    auto vertexBuffer = ctx->uploadData(kCubeVerts, sizeof(kCubeVerts));
    auto indexBuffer = ctx->uploadData(kCubeIndices, sizeof(kCubeIndices));

    rhi::VertexBufferView vertexBufferView {
        vertexBuffer.gpuAddress(),
        std::size(kCubeVerts),
        sizeof(Vertex)
    };

    rhi::IndexBufferView indexBufferView {
        indexBuffer.gpuAddress(),
        std::size(kCubeIndices),
        rhi::Format::uint32
    };

    mesh = {
        .pso = std::move(pipeline),

        .vertexBuffer = std::move(vertexBuffer),
        .indexBuffer = std::move(indexBuffer),

        .vertexBufferView = vertexBufferView,
        .indexBufferView = indexBufferView
    };

    commands.endRecording();

    return commands.get();
}

size_t BasicScene::cbvSetSize() const {
    return std::size(world->textures) + Slots::eTextures;
}
