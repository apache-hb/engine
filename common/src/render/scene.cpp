#include "engine/render/scene.h"
#include "engine/base/logging.h"
#include "engine/render/data.h"
#include "engine/render/render.h"
#include "engine/rhi/rhi.h"
#include "imgui.h"

#include <array>
#include <debugapi.h>

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
        { 0, rhi::Object::eTexture, rhi::BindingMutability::eBindless, SIZE_MAX }
    });

    constexpr auto kSceneBindings = std::to_array<rhi::Binding>({
        rhi::bindTable(rhi::ShaderVisibility::ePixel, kDebugBindings), // register(b0, space1) is debug data

        rhi::bindTable(rhi::ShaderVisibility::eAll, kSceneBufferBindings), // register(b0) is per scene data
        rhi::bindTable(rhi::ShaderVisibility::eVertex, kObjectBufferBindings), // register(b1) is per object data
        rhi::bindConst(rhi::ShaderVisibility::ePixel, 2, 1), // register(b2) is per primitive data
        rhi::bindTable(rhi::ShaderVisibility::ePixel, kSceneTextureBindings) // register(t0...) are all the textures
    });

    namespace DepthSlots {
        enum Slot : unsigned {
            eDepthStencil,
            eTotal
        };
    }
}

void SceneBufferHandle::update(Camera *camera, float aspectRatio, float3 light) {
    Super::update({
        .view = camera->getView(),
        .projection = camera->getProjection(aspectRatio),
        .camera = camera->mvp(float4x4::identity(), aspectRatio),
        .light = light
    });
}

BasicScene::BasicScene(Create&& info) 
    : RenderScene(info.resolution)
    , camera(info.camera)
{
    auto [width, height] = resolution;
    view = rhi::View(0, 0, float(width), float(height));
    aspectRatio = resolution.aspectRatio<float>();
}

size_t BasicScene::addTexture(const assets::Texture& image) {
    return update([&](auto) {
        auto it = ctx->uploadTexture(getTextureCpuHandle(textures.size()), image.size, image.data);
        textures.emplace_back(std::move(it));

        world.textures.push_back({ 
            .name = image.name, 
            .index = textures.size() - 1 
        });

        return world.textures.size() - 1;
    });
}

size_t BasicScene::addNode(const assets::Node& node) {
    return update([&](auto) {
        world.nodes.push_back(node);
        objectData.emplace_back(ctx->getDevice(), getObjectBufferCpuHandle(objectData.size()));
        return world.nodes.size() - 1;
    });
}

size_t BasicScene::addVertexBuffer(const assets::VertexBuffer& verts) {
    return update([&](auto) {
        auto vertexBuffer = ctx->uploadData(verts.data(), verts.size() * sizeof(assets::Vertex));

        rhi::VertexBufferView vertexBufferView = rhi::newVertexBufferView(vertexBuffer.gpuAddress(), verts.size(), sizeof(assets::Vertex));

        vertexBuffers.push_back(std::move(vertexBuffer));
        vertexBufferViews.push_back(vertexBufferView);
        return vertexBuffers.size() - 1;
    });
}

size_t BasicScene::addIndexBuffer(const assets::IndexBuffer& indices) {
    return update([&](auto) {
        size_t indexBufferSize = indices.size() * sizeof(uint32_t);

        auto indexBuffer = ctx->uploadData(indices.data(), indexBufferSize);

        rhi::IndexBufferView indexBufferView {
            .buffer = indexBuffer.gpuAddress(),
            .length = indices.size(),
            .format = rhi::Format::uint32
        };

        indexBuffers.push_back(std::move(indexBuffer));
        indexBufferViews.push_back(indexBufferView);
        
        world.indices.push_back(indices);
        return world.indices.size() - 1;
    });
}

size_t BasicScene::addPrimitive(const assets::Primitive& prim) {
    return update([&](auto) {
        world.primitives.emplace_back(prim);
        return world.primitives.size() - 1;
    });
}

ID3D12CommandList *BasicScene::populate() {
    std::lock_guard lock(mutex);
    if (ImGui::Begin("Heap")) {
        auto& alloc = ctx->getAlloc();

        ImGui::Text("Heap: %zu / %zu", alloc.getUsed(), alloc.getSize());

        if (ImGui::BeginListBox("Slots")) {
            for (size_t i = 0u; i < alloc.getUsed(); ++i) {
                ImGui::Text("%zu: %s", i, DescriptorSlot::getSlotName(alloc.getBit(i)));
            }

            ImGui::EndListBox();
        }
    }
    ImGui::End();

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

        if (ImGui::CollapsingHeader("Index Buffers")) {
            if (ImGui::BeginTable("Index Buffers", 1, ImGuiTableFlags_Borders)) {
                ImGui::TableSetupColumn("Size");
                for (auto& buffer : indexBufferViews) {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("%zu bytes", buffer.length);
                }
                ImGui::EndTable();
            }
        }

        if (ImGui::CollapsingHeader("Nodes")) {
            if (ImGui::BeginTable("Nodes", 2, ImGuiTableFlags_Borders)) {
                ImGui::TableSetupColumn("Name");
                ImGui::TableSetupColumn("Properties");
                ImGui::TableHeadersRow();

                for (auto& node : world.nodes) {
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
        }

        if (ImGui::CollapsingHeader("Textures")) {
            if (ImGui::BeginTable("Textures", 2, ImGuiTableFlags_Borders)) {
                ImGui::TableSetupColumn("Name");
                ImGui::TableSetupColumn("Value");
                ImGui::TableHeadersRow();

                for (size_t i = 0; i < world.textures.size(); ++i) {
                    auto& texture = world.textures[i];
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("%s", texture.name.c_str());
                    ImGui::TableNextColumn();
                    ImGui::Image((ImTextureID)getTextureGpuHandle(texture.index), { 128, 128 });
                }
                ImGui::EndTable();
            }
        }
    }
    ImGui::End();

    // update light and camera position
    sceneData.update(camera, aspectRatio, light);

    // update object data
    updateObject(0, float4x4::identity());

    commands.beginRecording(allocators[ctx->currentFrame()]);

    commands.setViewAndScissor(view);
    commands.setRenderTarget(ctx->getRenderTarget(), dsvHeap.cpuHandle(DepthSlots::eDepthStencil), kClearColour); // render to the intermediate framebuffer

    commands.setPipeline(pso);

    auto kDescriptors = std::to_array({ ctx->getHeap().get() });
    commands.bindDescriptors(kDescriptors);

    // bind data that wont change
    commands.bindTable(0, ctx->getCbvGpuHandle(debugBufferOffset)); // register(b0, space1) is debug data 
    commands.bindTable(1, ctx->getCbvGpuHandle(sceneBufferOffset)); // register(b0) is per scene data

    commands.bindTable(4, getTextureGpuHandle(0)); // register(t0...) are all the textures

    for (size_t i = 0; i < std::size(world.nodes); i++) {
        const auto& node = world.nodes[i];
        commands.bindTable(2, getObjectBufferGpuHandle(i)); // register(b1) is per object data

        for (size_t j : node.primitives) {
            const auto& primitive = world.primitives[j];
            commands.bindConst(3, 0, uint32_t(primitive.texture)); // register(b2) is per primitive data

            auto kBuffer = std::to_array({ vertexBufferViews[primitive.verts] });
            commands.setVertexBuffers(kBuffer);
            commands.drawMesh(indexBufferViews[primitive.indices]);
        }
    }

    commands.endRecording();

    return commands.get();
}

ID3D12CommandList *BasicScene::attach(Context *render) {
    ctx = render;

    auto& device = ctx->getDevice();
    auto& alloc = ctx->getAlloc();

    allocators = {ctx->frameCount()};
    for (size_t i = 0; i < ctx->frameCount(); i++) {
        allocators[i] = device.newAllocator(rhi::CommandList::Type::eDirect);
    }

    dsvHeap = device.newDescriptorSet(DepthSlots::eTotal, rhi::DescriptorSet::Type::eDepthStencil, false);

    depthBuffer = device.newDepthStencil(resolution, dsvHeap.cpuHandle(DepthSlots::eDepthStencil));

    debugBufferOffset = alloc.alloc(DescriptorSlot::eDebugBuffer);
    sceneBufferOffset = alloc.alloc(DescriptorSlot::eSceneBuffer);

    debugData.attach(device, ctx->getCbvCpuHandle(debugBufferOffset));
    sceneData.attach(device, ctx->getCbvCpuHandle(sceneBufferOffset));

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

    pso.rename("scene-pso");

    // TODO: dont hardcode values
    objectBufferOffset = alloc.alloc(DescriptorSlot::eObjectBuffer, 512);
    textureBufferOffset = alloc.alloc(DescriptorSlot::eTexture, 512);

    return commands.get();
}

void BasicScene::updateObject(size_t index, math::float4x4 parent) {
    auto &node = world.nodes[index];

    auto transform = parent * world.nodes[index].transform;
    
    if (world.clearDirty(index)) {
        objectData[index].update({ .transform = transform });
    }

    for (size_t child : node.children) {
        updateObject(child, transform);
    }
}

rhi::GpuHandle BasicScene::getObjectBufferGpuHandle(size_t index) {
    ASSERTF(
        ctx->getAlloc().checkBit(objectBufferOffset + index, DescriptorSlot::eObjectBuffer), 
        "expecting {} found {} instead at {}+{}", DescriptorSlot::getSlotName(DescriptorSlot::eObjectBuffer),
        DescriptorSlot::getSlotName(ctx->getAlloc().getBit(objectBufferOffset + index)), 
        objectBufferOffset, index
    );
    return ctx->getCbvGpuHandle(objectBufferOffset + index);
}

rhi::CpuHandle BasicScene::getObjectBufferCpuHandle(size_t index) {
    ASSERTF(
        ctx->getAlloc().checkBit(objectBufferOffset + index, DescriptorSlot::eObjectBuffer), 
        "expecting {} found {} instead at {}+{}", DescriptorSlot::getSlotName(DescriptorSlot::eObjectBuffer),
        DescriptorSlot::getSlotName(ctx->getAlloc().getBit(objectBufferOffset + index)), 
        objectBufferOffset, index
    );
    return ctx->getCbvCpuHandle(objectBufferOffset + index);
}

rhi::GpuHandle BasicScene::getTextureGpuHandle(size_t index) {
    ASSERTF(
        ctx->getAlloc().checkBit(textureBufferOffset + index, DescriptorSlot::eTexture), 
        "expecting {} found {} instead at {}+{}", DescriptorSlot::getSlotName(DescriptorSlot::eTexture),
        DescriptorSlot::getSlotName(ctx->getAlloc().getBit(textureBufferOffset + index)), 
        textureBufferOffset, index
    );
    return ctx->getCbvGpuHandle(textureBufferOffset + index);
}

rhi::CpuHandle BasicScene::getTextureCpuHandle(size_t index) {
    ASSERTF(
        ctx->getAlloc().checkBit(textureBufferOffset + index, DescriptorSlot::eTexture), 
        "expecting {} found {} instead at {}+{}", DescriptorSlot::getSlotName(DescriptorSlot::eTexture),
        DescriptorSlot::getSlotName(ctx->getAlloc().getBit(textureBufferOffset + index)), 
        textureBufferOffset, index
    );
    return ctx->getCbvCpuHandle(textureBufferOffset + index);
}
