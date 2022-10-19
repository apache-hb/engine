#include "engine/render/scene.h"
#include "engine/render/data.h"
#include "engine/rhi/rhi.h"
#include "imgui.h"

#include <array>

using namespace simcoe;
using namespace simcoe::render;

namespace {
    constexpr auto kInputLayout = std::to_array<rhi::InputElement>({
        { "POSITION", rhi::Format::float32x3, offsetof(assets::Vertex, position) },
        { "NORMAL", rhi::Format::float32x3, offsetof(assets::Vertex, normal) },
        { "TEXCOORD", rhi::Format::float32x2, offsetof(assets::Vertex, uv) }
    });

    constexpr auto kSamplers = std::to_array<rhi::Sampler>({
        { rhi::ShaderVisibility::ePixel }
    });

    constexpr auto kDebugBindings = std::to_array<rhi::BindingRange>({
        { .base = 0, .type = rhi::Object::eConstBuffer, .mutability = rhi::BindingMutability::eAlwaysStatic, .space = 1 }
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
        rhi::bindTable(rhi::ShaderVisibility::ePixel, kDebugBindings), // register(b0, space1) is debug data

        rhi::bindTable(rhi::ShaderVisibility::eAll, kSceneBufferBindings), // register(b0) is per scene data
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
            eDebugBuffer, // debug data
            eSceneBuffer, // camera matrix
            eTotal
        };
    }

    namespace DepthSlots {
        enum Slot : unsigned {
            eDepthStencil,
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

void SceneBufferHandle::update(Camera *camera, float aspectRatio, float3 light) {
    Super::update({
        .camera = camera->mvp(float4x4::identity(), aspectRatio),
        .light = light
    });
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
    if (ImGui::Begin("World")) {
        float data[] = { light.x, light.y, light.z };
        ImGui::SliderFloat3("Light", data, -5.f, 5.f);
        light = { data[0], data[1], data[2] };

        {
            auto& dbg = debugData.get();
            bool bNormalDebug = dbg.debugFlags & kDebugNormals;
            bool bTexcoordDebug = dbg.debugFlags & kDebugUVs;

            ImGui::Checkbox("Debug Normals", &bNormalDebug);
            ImGui::Checkbox("Debug UVs", &bTexcoordDebug);

            debugData.update({
                .debugFlags = (bNormalDebug ? kDebugNormals : 0) | (bTexcoordDebug ? kDebugUVs : 0)
            });
        }

        ImGui::Text("Nodes");
        if (ImGui::BeginTable("Nodes", 2, ImGuiTableFlags_Borders)) {
            ImGui::TableSetupColumn("Name");
            ImGui::TableSetupColumn("Properties");
            ImGui::TableHeadersRow();

            for (auto& node : world->nodes) {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text("%s", node.name.c_str());

                ImGui::TableNextColumn();

                {
                    auto [x, y, z] = node.transform.scale();
                    float scale[3] = { x, y, z };
                    ImGui::SliderFloat3("Scale", scale, 0.0f, 1.0f);
                    node.transform.setScale({ scale[0], scale[1], scale[2] });
                }

                {
                    auto [x, y, z] = node.transform.translation();
                    float translation[3] = { x, y, z };
                    ImGui::SliderFloat3("Translation", translation, -1.0f, 1.0f);
                    node.transform.setTranslation({ translation[0], translation[1], translation[2] });
                }
            }
            ImGui::EndTable();
        }

        ImGui::Text("Textures");
        if (ImGui::BeginTable("Textures", 2, ImGuiTableFlags_Borders)) {
            ImGui::TableSetupColumn("Name");
            ImGui::TableSetupColumn("Value");
            ImGui::TableHeadersRow();

            for (size_t i = 0; i < world->textures.size(); ++i) {
                auto& texture = world->textures[i];
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text("%s", texture.name.c_str());
                ImGui::TableNextColumn();
                //ImGui::Image((ImTextureID)getTextureGpuHandle(i), { 128, 128 });
            }
            ImGui::EndTable();
        }
    }
    ImGui::End();

    // update light position
    sceneData.update(camera, aspectRatio, light);

    // update object data
    updateObject(0, float4x4::identity());

    commands.beginRecording(allocators[ctx->currentFrame()]);

    commands.setPipeline(pso);

    commands.setViewAndScissor(view);
    commands.setRenderTarget(ctx->getRenderTarget(), dsvHeap.cpuHandle(DepthSlots::eDepthStencil), kClearColour); // render to the intermediate framebuffer

    auto kDescriptors = std::to_array({ cbvHeap.get() });
    commands.bindDescriptors(kDescriptors);

    // bind data that wont change
    commands.bindTable(0, cbvHeap.gpuHandle(Slots::eDebugBuffer));
    commands.bindTable(1, cbvHeap.gpuHandle(Slots::eSceneBuffer));
    commands.bindTable(4, getTextureGpuHandle(0));

    for (size_t i = 0; i < std::size(world->nodes); i++) {
        const auto& node = world->nodes[i];
        commands.bindTable(2, getObjectBufferGpuHandle(i));

        for (size_t j : node.primitives) {
            const auto& primitive = world->primitives[j];
            commands.bindConst(3, 0, uint32_t(primitive.texture));

            auto kBuffer = std::to_array({ vertexBufferViews[primitive.verts] });
            commands.setVertexBuffers(kBuffer);
            commands.drawMesh(indexBufferViews[primitive.indices]);
        }
    }

    commands.endRecording();

    return commands.get();
}

ID3D12CommandList *BasicScene::attach(Context *ctx) {
    auto &device = ctx->getDevice();

    allocators = {ctx->frameCount()};
    for (size_t i = 0; i < ctx->frameCount(); i++) {
        allocators[i] = device.newAllocator(rhi::CommandList::Type::eDirect);
    }

    cbvHeap = device.newDescriptorSet(cbvSetSize(), rhi::DescriptorSet::Type::eConstBuffer, true);
    dsvHeap = device.newDescriptorSet(DepthSlots::eTotal, rhi::DescriptorSet::Type::eDepthStencil, false);

    depthBuffer = device.newDepthStencil(resolution, dsvHeap.cpuHandle(DepthSlots::eDepthStencil));

    debugData.attach(device, cbvHeap.cpuHandle(Slots::eDebugBuffer));
    sceneData.attach(device, cbvHeap.cpuHandle(Slots::eSceneBuffer));

    commands = device.newCommandList(allocators[ctx->currentFrame()], rhi::CommandList::Type::eDirect);

    auto ps = loadShader("resources/shaders/scene-shader.ps.pso");
    auto vs = loadShader("resources/shaders/scene-shader.vs.pso");

    pso = device.newPipelineState({
        .samplers = kSamplers,
        .bindings = kSceneBindings,
        .input = kInputLayout,
        .ps = ps,
        .vs = vs,
        .depth = true
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

    // upload index buffers
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
    objectData[index].update({ .transform = transform, .texture = 0 });
    for (size_t child : node.children) {
        updateObject(child, transform);
    }
}

size_t BasicScene::cbvSetSize() const {
    return Slots::eTotal + std::size(world->nodes) + std::size(world->textures);
}

rhi::GpuHandle BasicScene::getObjectBufferGpuHandle(size_t index) {
    return cbvHeap.gpuHandle(Slots::eTotal + index);
}

rhi::CpuHandle BasicScene::getObjectBufferCpuHandle(size_t index) {
    return cbvHeap.cpuHandle(Slots::eTotal + index);
}

rhi::GpuHandle BasicScene::getTextureGpuHandle(size_t index) {
    return cbvHeap.gpuHandle(Slots::eTotal + std::size(world->nodes) + index);
}

rhi::CpuHandle BasicScene::getTextureCpuHandle(size_t index) {
    return cbvHeap.cpuHandle(Slots::eTotal + std::size(world->nodes) + index);
}
