#include "engine/render/scene.h"

#include <array>

using namespace engine;
using namespace engine::render;

namespace {
    constexpr auto kInputLayout = std::to_array<rhi::InputElement>({
        { "POSITION", rhi::Format::float32x3, offsetof(assets::Vertex, position) },
        { "TEXCOORD", rhi::Format::float32x2, offsetof(assets::Vertex, uv) }
    });

    constexpr auto kSamplers = std::to_array<rhi::Sampler>({
        { rhi::ShaderVisibility::ePixel }
    });

    constexpr auto kSceneBufferBindings = std::to_array<rhi::BindingRange>({
        { 0, rhi::Object::eConstBuffer, rhi::BindingMutability::eStaticAtExecute }
    });

    constexpr auto kObjectBufferBindings = std::to_array<rhi::BindingRange>({
        { 1, rhi::Object::eConstBuffer, rhi::BindingMutability::eStaticAtExecute }
    });

    constexpr auto kSceneTextureBindings = std::to_array<rhi::BindingRange>({
        { 0, rhi::Object::eTexture, rhi::BindingMutability::eAlwaysStatic, SIZE_MAX }
    });

    constexpr auto kSceneBindings = std::to_array<rhi::Binding>({
        rhi::bindTable(rhi::ShaderVisibility::eVertex, kSceneBufferBindings), // register(b0) is per scene data
        rhi::bindTable(rhi::ShaderVisibility::eVertex, kObjectBufferBindings), // register(b1) is per object data
        rhi::bindConst(rhi::ShaderVisibility::ePixel, 2, 1), // register(b2) is per primitive data
        rhi::bindTable(rhi::ShaderVisibility::ePixel, kSceneTextureBindings) // register(t0...) are all the textures
    });

    /**
     * table layout:
     * 0: scene buffer
     * 0..N: per object object buffers
     * N..T: texture views
     */
    namespace Slots {
        enum Slot : unsigned {
            eSceneBuffer, // camera matrix
            eTotal
        };
    }

    constexpr assets::Vertex kCubeVerts[] = {
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

void SceneBufferHandle::attach(rhi::Device& device, rhi::CpuHandle handle) {
    buffer = device.newBuffer(sizeof(SceneBuffer), rhi::DescriptorSet::Visibility::eHostVisible, rhi::Buffer::eUpload);
    device.createConstBufferView(buffer, sizeof(SceneBuffer), handle);    
    
    ptr = buffer.map();
}

void SceneBufferHandle::update(Camera *camera, float aspectRatio) {
    data.camera = camera->mvp(float4x4::identity(), aspectRatio);
    memcpy(ptr, &data, sizeof(SceneBuffer));
}

ObjectBufferHandle::ObjectBufferHandle(rhi::Device& device, rhi::CpuHandle handle) {
    buffer = device.newBuffer(sizeof(SceneBuffer), rhi::DescriptorSet::Visibility::eHostVisible, rhi::Buffer::eUpload);
    device.createConstBufferView(buffer, sizeof(ObjectBuffer), handle);

    ptr = buffer.map();
}

void ObjectBufferHandle::update(math::float4x4 transform) {
    data = { .transform = transform, .texture = 0 };
    memcpy(ptr, &data, sizeof(ObjectBuffer));
}

BasicScene::BasicScene(Create&& info) 
    : RenderScene(info.resolution)
    , camera(info.camera)
    , world(info.world)
{
    auto [width, height] = resolution;
    view = rhi::View(0, 0, float(width), float(height));
    aspectRatio = resolution.aspectRatio<float>();
}

ID3D12CommandList *BasicScene::populate(Context *ctx) {
    sceneData.update(camera, aspectRatio);
    for (size_t i = 0; i < std::size(world->nodes); i++) {
        const auto& node = world->nodes[i];
        if (node.children.size()) continue;
        
        updateObject(i, float4x4::identity());
    }

    commands.beginRecording(allocators[ctx->currentFrame()]);

    commands.setPipeline(pso);

    commands.setViewAndScissor(view);
    commands.setRenderTarget(ctx->getRenderTarget(), kClearColour); // render to the intermediate framebuffer

    auto kDescriptors = std::to_array({ heap.get() });
    commands.bindDescriptors(kDescriptors);

    commands.setVertexBuffers(vertexBufferViews);

    // bind data that wont change
    commands.bindTable(0, heap.gpuHandle(Slots::eSceneBuffer));
    commands.bindTable(3, getTextureGpuHandle(0));

    for (size_t i = 0; i < std::size(world->nodes); i++) {
        const auto& node = world->nodes[i];
        commands.bindTable(1, getObjectBufferGpuHandle(i));

        for (size_t j : node.primitives) {
            const auto& primitive = world->primitives[j];
            commands.bindConst(2, 0, uint32_t(primitive.texture));
            commands.drawMesh(indexBufferViews[primitive.indices]);
            // TODO: actually draw primitive
        }
    }

    commands.endRecording();

    return commands.get();
}

ID3D12CommandList *BasicScene::attach(Context *ctx) {
    auto &device = ctx->getDevice();

    for (size_t i = 0; i < kFrameCount; i++) {
        allocators[i] = device.newAllocator(rhi::CommandList::Type::eDirect);
    }

    heap = device.newDescriptorSet(cbvSetSize(), rhi::DescriptorSet::Type::eConstBuffer, true);

    sceneData.attach(device, heap.cpuHandle(Slots::eSceneBuffer));

    commands = device.newCommandList(allocators[ctx->currentFrame()], rhi::CommandList::Type::eDirect);

    auto ps = loadShader("resources/shaders/scene-shader.ps.pso");
    auto vs = loadShader("resources/shaders/scene-shader.vs.pso");

    pso = device.newPipelineState({
        .samplers = kSamplers,
        .bindings = kSceneBindings,
        .input = kInputLayout,
        .ps = ps,
        .vs = vs
    });

    commands.beginRecording(allocators[ctx->currentFrame()]);

    // upload all textures
    for (size_t i = 0; i < std::size(world->textures); i++) {
        const auto &image = world->textures[i];
        auto it = ctx->uploadTexture(commands, image.size, image.data);
        device.createTextureBufferView(it, getTextureCpuHandle(i));
        textures.push_back(std::move(it));
    }

    // upload all our node data
    for (size_t i = 0; i < std::size(world->nodes); i++) {
        objectData.emplace_back(device, getObjectBufferCpuHandle(i));        
    }

    // upload all our vertex buffers
    for (const auto& data : world->verts) {
        size_t vertexBufferSize = data.size() * sizeof(assets::Vertex);
        
        auto vertexBuffer = ctx->uploadData(data.data(), vertexBufferSize);

        rhi::VertexBufferView vertexBufferView = rhi::newVertexBufferView(vertexBuffer.gpuAddress(), data.size(), sizeof(assets::Vertex));

        vertexBuffers.push_back(std::move(vertexBuffer));
        vertexBufferViews.push_back(vertexBufferView);
    }

    for (const auto& indices : world->indices) {
        size_t indexBufferSize = indices.size() * sizeof(uint32_t);

        auto indexBuffer = ctx->uploadData(indices.data(), indexBufferSize);

        rhi::IndexBufferView indexBufferView {
            .buffer = indexBuffer.gpuAddress(),
            .size = indices.size(),
            .format = rhi::Format::uint32
        };

        indexBuffers.push_back(std::move(indexBuffer));
        indexBufferViews.push_back(indexBufferView);
    }

    commands.endRecording();

    return commands.get();
}

void BasicScene::updateObject(size_t index, math::float4x4 parent) {
    const auto &node = world->nodes[index];
    
    auto transform = parent * world->nodes[index].transform;
    objectData[index].update(transform);
    for (size_t child : node.children) {
        updateObject(child, transform);
    }
}

size_t BasicScene::cbvSetSize() const {
    return Slots::eTotal + std::size(world->nodes) + std::size(world->textures);
}

rhi::GpuHandle BasicScene::getObjectBufferGpuHandle(size_t index) {
    return heap.gpuHandle(Slots::eTotal + index);
}

rhi::CpuHandle BasicScene::getObjectBufferCpuHandle(size_t index) {
    return heap.cpuHandle(Slots::eTotal + index);
}

rhi::GpuHandle BasicScene::getTextureGpuHandle(size_t index) {
    return heap.gpuHandle(Slots::eTotal + std::size(world->nodes) + index);
}

rhi::CpuHandle BasicScene::getTextureCpuHandle(size_t index) {
    return heap.cpuHandle(Slots::eTotal + std::size(world->nodes) + index);
}
