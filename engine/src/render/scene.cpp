#include "engine/render/scene.h"

#include <array>

#include "stb_image.h"

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
        { 0, rhi::Object::eConstBuffer, rhi::BindingMutability::eStaticAtExecute }
    });

    constexpr auto scenePixelRanges = std::to_array<rhi::BindingRange>({
        { 0, rhi::Object::eTexture, rhi::BindingMutability::eAlwaysStatic }
    });

    constexpr auto sceneBindings = std::to_array<rhi::BindingTable>({
        rhi::BindingTable {
            .visibility = rhi::ShaderVisibility::eVertex,
            .ranges = sceneVertexRanges
        },
        rhi::BindingTable {
            .visibility = rhi::ShaderVisibility::ePixel,
            .ranges = scenePixelRanges
        }
    });

    struct Image {
        std::vector<std::byte> data;
        rhi::TextureSize size;
    };

    Image loadImage(const char *path, logging::Channel *channel) {
        int x, y, channels;
        auto *it = reinterpret_cast<std::byte*>(stbi_load(path, &x, &y, &channels, 4));

        if (it == nullptr) {
            channel->fatal("failed to load image at {}: {}", path, stbi_failure_reason());
            return Image { };
        }

        return Image {
            .data = std::vector(it, it + (x * y * 4)),
            .size = { size_t(x), size_t(y) }
        };
    }

    namespace Slots {
        enum Slot : unsigned {
            eCamera, // camera matrix
            eTexture, // object texture
            eTotal
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

BasicScene::BasicScene(Create&& info) : Scene(info.resolution), camera(info.camera) {
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
    commands.bindTable(1, heap.gpuHandle(Slots::eTexture));

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

    heap = device.newDescriptorSet(Slots::eTotal, rhi::DescriptorSet::Type::eConstBuffer, true);

    camera.attach(device, heap.cpuHandle(Slots::eCamera));

    commands = device.newCommandList(allocators[ctx->currentFrame()], rhi::CommandList::Type::eDirect);

    auto ps = loadShader("resources/shaders/scene-shader.ps.pso");
    auto vs = loadShader("resources/shaders/scene-shader.vs.pso");
    auto [image, size] = loadImage("C:\\Users\\elliot\\Downloads\\uv-coords.jpg", ctx->getChannel());

    auto pipeline = device.newPipelineState({
        .samplers = samplers,
        .tables = sceneBindings,
        .input = inputLayout,
        .ps = ps,
        .vs = vs
    });

    commands.beginRecording(allocators[ctx->currentFrame()]);

    texture = ctx->uploadTexture(commands, size, image);
    device.createTextureBufferView(texture, heap.cpuHandle(Slots::eTexture));

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
