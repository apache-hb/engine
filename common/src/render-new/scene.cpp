#include "engine/render-new/scene.h"
#include "engine/render-new/render.h"

#include "engine/assets/assets.h"
#include "imgui/imgui.h"
#include "imnodes/imnodes.h"

#include <array>

using namespace simcoe;
using namespace simcoe::render;

namespace {
    constexpr math::float4 kClearColour = { 0.f, 0.2f, 0.4f, 1.f };
    constexpr math::float4 kLetterBox = { 0.f, 0.f, 0.f, 1.f };
}

struct BufferResource : Resource {
    virtual rhi::Buffer& get() = 0;
    size_t cbvHandle() const { return cbvOffset; }

protected:
    BufferResource(const Info& info, size_t handle) 
        : Resource(info)
        , cbvOffset(handle) 
    { }

    void addBarrier(Barriers& barriers, Output* output, Input* input) override {
        Resource::addBarrier(barriers, output, input);

        auto before = output->getState();
        auto after = input->getState();

        ASSERT(before != State::eInvalid && after != State::eInvalid);
        if (before == after) { return; }

        barriers.push_back(rhi::newStateTransition(get(), before, after));
    }

private:
    size_t cbvOffset;
};

struct RenderTargetResource final : BufferResource {
    RenderTargetResource(const Info& info)
        : BufferResource(info, SIZE_MAX)
    { }

    rhi::Buffer& get() override { return getContext().getRenderTarget(); }
    rhi::CpuHandle rtvCpuHandle() { return getContext().getRenderTargetHandle(); }
};

struct TextureResource : BufferResource {
    TextureResource(const Info& info, State initial, rhi::TextureSize size, bool hostVisible)
        : BufferResource(info, ::getContext(info).getHeap().alloc(DescriptorSlot::eTexture))
    { 
        auto& ctx = getContext();
        auto& heap = ctx.getHeap();
        auto& device = ctx.getDevice();

        const auto desc = rhi::newTextureDesc(size, initial);
        const auto visibility = hostVisible ? rhi::DescriptorSet::Visibility::eHostVisible : rhi::DescriptorSet::Visibility::eDeviceOnly;

        buffer = device.newTexture(desc, visibility, kClearColour);
        device.createTextureBufferView(get(), heap.cpuHandle(cbvHandle(), 1, DescriptorSlot::eTexture));

        buffer.rename(getName());
    }

    rhi::Buffer& get() override { return buffer; }

    rhi::CpuHandle cbvCpuHandle() const { return getContext().getHeap().cpuHandle(cbvHandle(), 1, DescriptorSlot::eTexture); }
    rhi::GpuHandle cbvGpuHandle() const { return getContext().getHeap().gpuHandle(cbvHandle(), 1, DescriptorSlot::eTexture); }

private:
    rhi::Buffer buffer;
};

struct SceneTargetResource final : TextureResource {
    SceneTargetResource(const Info& info, State initial)
        : TextureResource(info, initial, ::getContext(info).sceneSize(), false)
    { 
        auto& ctx = getContext();
        auto& device = ctx.getDevice();

        rtvOffset = ctx.getPresentQueue().getSceneHandle();
        device.createRenderTargetView(get(), rtvOffset);
    }

    rhi::CpuHandle rtvHandle() const { return rtvOffset; }

private:
    rhi::CpuHandle rtvOffset;
};

struct GlobalPass final : Pass {
    GlobalPass(const Info& info) : Pass(info, CommandSlot::ePost) {
        rtvResource = getParent()->addResource<RenderTargetResource>("rtv");
        sceneTargetResource = getParent()->addResource<SceneTargetResource>("scene-target", State::eRenderTarget);

        renderTargetOut = newOutput<Source>("rtv", State::ePresent, rtvResource);
        sceneTargetOut = newOutput<Source>("scene-target", State::eRenderTarget, sceneTargetResource);
    }

    void execute(Context& ctx) override { 
        ctx.beginFrame();
    }

    void init(Context&) override {
        ImNodes::CreateContext();

        auto& passes = getParent()->passes;
        int i = 0;
        for (auto& [name, pass] : passes) {
            passIds[pass.get()] = i++;
            for (auto& input : pass->inputs) {
                wireIds[input.get()] = i++;
            }

            for (auto& output : pass->outputs) {
                wireIds[output.get()] = i++;
            }
        }

        int l = 0;
        for (auto& [source, dst] : getParent()->wires) {
            auto from = wireIds[source];
            auto to = wireIds[dst];

            links[l++] = { from, to };
        }
    }

    void deinit(Context&) override {
        ImNodes::DestroyContext();
    }

    void imgui(Context& ctx) override {
        if (ImGui::Begin("Context state")) {
            ImGui::Text("Total frames: %zu", ctx.getFrames());
            ImGui::Text("Frame: %zu", ctx.currentFrame());

            auto [wx, wy] = ctx.windowSize();
            ImGui::Text("Present resolution: %zu x %zu", wx, wy);

            auto [sx, sy] = ctx.sceneSize();
            ImGui::Text("Scene resolution: %zu x %zu", sx, sy);

            auto& heap = ctx.getHeap();
            ImGui::Text("CBV Heap: %zu/%zu", heap.getUsed(), heap.getSize());

            if (ImGui::BeginListBox("Slots")) {
                for (size_t i = 0; i < heap.getUsed(); i++) {
                    ImGui::Text("Slot %zu: %s", i, getSlotName(heap.getBit(i)));
                }
                ImGui::EndListBox();
            }
        }

        ImGui::End();

        if (ImGui::Begin("Graph state")) {
            ImNodes::BeginNodeEditor();

            for (auto& [name, pass] : getParent()->passes) {
                ImNodes::BeginNode(passIds[pass.get()]);
                
                ImNodes::BeginNodeTitleBar();
                ImGui::TextUnformatted(name);
                ImNodes::EndNodeTitleBar();

                for (auto& input : pass->inputs) {
                    ImNodes::BeginInputAttribute(wireIds[input.get()]);
                    ImGui::TextUnformatted(input->getName());
                    ImNodes::EndInputAttribute();
                }

                for (auto& output : pass->outputs) {
                    ImNodes::BeginOutputAttribute(wireIds[output.get()]);
                    ImGui::TextUnformatted(output->getName());
                    ImNodes::EndOutputAttribute();
                }

                ImNodes::EndNode();
            }

            for (auto& [id, link] : links) {
                auto [from, to] = link;
                ImNodes::Link(id, from, to);
            }

            ImNodes::EndNodeEditor();
        }

        ImGui::End();
    }

    RenderTargetResource *rtvResource;
    SceneTargetResource *sceneTargetResource;

    Output *renderTargetOut;
    Output *sceneTargetOut;

private:
    struct Link {
        int source;
        int destination;
    };

    std::unordered_map<Pass*, int> passIds;
    std::unordered_map<Wire*, int> wireIds;
    std::unordered_map<int, Link> links;
};

struct PresentPass final : Pass {
    PresentPass(const Info& info) : Pass(info, CommandSlot::ePost) {
        renderTargetIn = newInput<Input>("rtv", State::ePresent);
        sceneTargetIn = newInput<Input>("scene-target", State::eRenderTarget);
    }

    void execute(Context& ctx) override {
        ctx.endFrame();
        ctx.present();
    }
    
    Input *renderTargetIn;
    Input *sceneTargetIn;
};

struct ScenePass final : Pass, assets::IWorldSink {
    ScenePass(const Info& info) : Pass(info, CommandSlot::eScene) {
        sceneTargetIn = newInput<Input>("scene-target", State::eRenderTarget);
        sceneTargetOut = newOutput<Relay>("scene-target", sceneTargetIn);
    }

    void addWorld(const char* path) {
        loadGltfAsync(path);
    }

    void init(Context& ctx) override { 
        auto& device = ctx.getDevice();
        auto& heap = ctx.getHeap();

        vs = loadShader("resources/shaders/scene-shader.vs.pso");
        ps = loadShader("resources/shaders/scene-shader.ps.pso");

        pso = device.newPipelineState({
            .samplers = kSamplers,
            .bindings = kSceneBindings,
            .input = kInputLayout,
            .ps = ps,
            .vs = vs,
            .depth = true
        });

        textureHeapOffset = heap.alloc(DescriptorSlot::eTexture, kMaxTextures);
    }

    void execute(Context& ctx) override {
        auto& cmd = ctx.getCommands(CommandSlot::eScene);

        auto [width, height] = ctx.sceneSize().as<float>();

        cmd.setPipeline(pso);
        cmd.setViewAndScissor(rhi::View(0, 0, width, height));
        cmd.clearRenderTarget(sceneTargetIn->rtvHandle(), kClearColour);
    }

    constexpr static uint8_t kMinColumns = 2;
    constexpr static uint8_t kMaxColumns = 8;
    constexpr static size_t kMaxTextures = 128;

    void imgui(Context&) override {
        if (ImGui::Begin("Scene data")) {
            ImGui::Text("Texture budget: %zu/%zu", usedTextures.load(), kMaxTextures);
            ImGui::SameLine();
            ImGui::SliderScalar("Columns", ImGuiDataType_U8, &columns, &kMinColumns, &kMaxColumns);

            if (ImGui::BeginTable("Textures", columns)) {
                ImVec2 avail = ImGui::GetContentRegionAvail();
                for (size_t i = 0; i < usedTextures; i++) {
                    ImGui::TableNextColumn();
                    ImGui::Button("Button", ImVec2(avail.x / columns, avail.x / columns));
                    auto [x, y] = textures[i].data.size;
                    ImGui::Text("%s (%zu) (%zu x %zu)", textures[i].data.name.c_str(), i, x, y);
                }
                ImGui::EndTable();
            }
        }
        ImGui::End();
    }

    WireHandle<Input, SceneTargetResource> sceneTargetIn;
    Output *sceneTargetOut;

    bool reserveTextures(size_t size) override {
        if (textures.size() + size > kMaxTextures) {
            return false;
        }

        textures.reserve(textures.size() + size);
        return true;
    }

    bool reserveNodes(size_t) override {
        return true;
    }

    size_t addVertexBuffer(assets::VertexBuffer&&) override {
        return 0;
    }

    size_t addIndexBuffer(assets::IndexBuffer&&) override {
        return 0;
    }

    size_t addTexture(const assets::Texture& texture) override {
        // TODO: actually upload and create textureViews for the data
        textures.push_back({
            .data = texture,
            .heapOffset = textureHeapOffset++
        });

        return usedTextures++;
    }

    size_t addPrimitive(const assets::Primitive&) override {
        return 0;
    }

    size_t addNode(const assets::Node&) override {
        return 0;
    }

private:
    Shader vs;
    Shader ps;

    rhi::PipelineState pso;

    uint8_t columns = 4;

    std::atomic_size_t textureHeapOffset = 0;

    struct TextureHandle {
        assets::Texture data;
        size_t heapOffset;
    };

    std::vector<assets::VertexBuffer> vbos;
    std::vector<assets::IndexBuffer> ibos;

    std::atomic_size_t usedTextures = 0;
    std::vector<TextureHandle> textures;

    std::vector<assets::Primitive> primitives;

    std::vector<assets::Node> nodes;

    constexpr static auto kSamplers = std::to_array<rhi::Sampler>({
        { rhi::ShaderVisibility::ePixel }
    });

    constexpr static auto kDebugBindings = std::to_array<rhi::BindingRange>({
        { .base = 0, .type = rhi::Object::eConstBuffer, .mutability = rhi::BindingMutability::eAlwaysStatic, .space = 1 }
    });

    constexpr static auto kSceneBufferBindings = std::to_array<rhi::BindingRange>({
        { 0, rhi::Object::eConstBuffer, rhi::BindingMutability::eStaticAtExecute }
    });

    constexpr static auto kObjectBufferBindings = std::to_array<rhi::BindingRange>({
        { 1, rhi::Object::eConstBuffer, rhi::BindingMutability::eStaticAtExecute }
    });

    constexpr static auto kSceneTextureBindings = std::to_array<rhi::BindingRange>({
        { 0, rhi::Object::eTexture, rhi::BindingMutability::eBindless, SIZE_MAX }
    });

    constexpr static auto kSceneBindings = std::to_array<rhi::Binding>({
        rhi::bindTable(rhi::ShaderVisibility::ePixel, kDebugBindings), // register(b0, space1) is debug data

        rhi::bindTable(rhi::ShaderVisibility::eAll, kSceneBufferBindings), // register(b0) is per scene data
        rhi::bindTable(rhi::ShaderVisibility::eVertex, kObjectBufferBindings), // register(b1) is per object data
        rhi::bindConst(rhi::ShaderVisibility::ePixel, 2, 1), // register(b2) is per primitive data
        rhi::bindTable(rhi::ShaderVisibility::ePixel, kSceneTextureBindings) // register(t0...) are all the textures
    });

    constexpr static auto kInputLayout = std::to_array<rhi::InputElement>({
        { "POSITION", rhi::Format::float32x3, offsetof(assets::Vertex, position) },
        { "NORMAL", rhi::Format::float32x3, offsetof(assets::Vertex, normal) },
        { "TEXCOORD", rhi::Format::float32x2, offsetof(assets::Vertex, uv) }
    });
};

struct PostPass final : Pass {
    PostPass(const Info& info) : Pass(info, CommandSlot::ePost) {
        sceneTargetIn = newInput<Input>("scene-target", State::ePixelShaderResource);
        sceneTargetOut = newOutput<Relay>("scene-target", sceneTargetIn); // TODO: this is also a hack, graph code should be able to handle this
        
        renderTargetIn = newInput<Input>("rtv", State::eRenderTarget);
        renderTargetOut = newOutput<Relay>("rtv", renderTargetIn);
    }

    void init(Context& ctx) override {
        initView(ctx);
        initScreenQuad(ctx);
    }

    void execute(Context& ctx) override {
        auto& cmd = ctx.getCommands(getSlot());
        auto *sceneTarget = sceneTargetIn.get();
        auto *renderTarget = renderTargetIn.get();

        cmd.setViewAndScissor(view);
        cmd.setRenderTarget(renderTarget->rtvCpuHandle(), rhi::CpuHandle::Invalid, kLetterBox);
        cmd.setPipeline(pso);

        cmd.bindTable(0, sceneTarget->cbvGpuHandle());
        cmd.setVertexBuffers(std::to_array({ vboView }));
        cmd.drawMesh(iboView);
    }

    WireHandle<Input, SceneTargetResource> sceneTargetIn;
    WireHandle<Input, RenderTargetResource> renderTargetIn;

    Output *renderTargetOut;
    Output *sceneTargetOut;

private:
    void initView(Context& ctx) {
        // TODO: how did this get off centerd?
        auto [sceneWidth, sceneHeight] = ctx.sceneSize();
        auto [windowWidth, windowHeight] = ctx.windowSize();

        auto widthRatio = float(sceneWidth) / windowWidth;
        auto heightRatio = float(sceneHeight) / windowHeight;

        float x = 1.f;
        float y = 1.f;

        if (widthRatio < heightRatio) {
            x = widthRatio / heightRatio;
        } else {
            y = heightRatio / widthRatio;
        }

        view.viewport.TopLeftX = windowWidth * (1.f - x) / 2.f;
        view.viewport.TopLeftY = windowHeight * (1.f - y) / 2.f;
        view.viewport.Width = x * windowWidth;
        view.viewport.Height = y * windowHeight;

        view.scissor.left = LONG(view.viewport.TopLeftX);
        view.scissor.right = LONG(view.viewport.TopLeftX + view.viewport.Width);
        view.scissor.top = LONG(view.viewport.TopLeftY);
        view.scissor.bottom = LONG(view.viewport.TopLeftY + view.viewport.Height);
    }

    void initScreenQuad(Context& ctx) {
        auto& device = ctx.getDevice();
        auto& copy = ctx.getCopyQueue();
        auto pending = copy.getThread();

        auto ps = loadShader("resources/shaders/post-shader.ps.pso");
        auto vs = loadShader("resources/shaders/post-shader.vs.pso");

        pso = device.newPipelineState({
            .samplers = kSamplers,
            .bindings = kBindings,
            .input = kInput,
            .ps = ps,
            .vs = vs
        });

        vbo = pending.uploadData(kScreenQuad, sizeof(kScreenQuad));
        ibo = pending.uploadData(kScreenQuadIndices, sizeof(kScreenQuadIndices));

        vboView = rhi::newVertexBufferView(vbo.gpuAddress(), std::size(kScreenQuad), sizeof(assets::Vertex));
        iboView = {
            .buffer = ibo.gpuAddress(),
            .length = std::size(kScreenQuadIndices),
            .format = rhi::Format::uint32
        };

        copy.submit(pending);
    }

    rhi::View view;
    
    rhi::PipelineState pso;
    rhi::Buffer vbo;
    rhi::Buffer ibo;
    rhi::VertexBufferView vboView;
    rhi::IndexBufferView iboView;

    constexpr static assets::Vertex kScreenQuad[] = {
        { .position = { -1.0f, -1.0f, 0.0f }, .uv = { 0.0f, 1.0f } }, // bottom left
        { .position = { -1.0f, 1.0f, 0.0f },  .uv = { 0.0f, 0.0f } },  // top left
        { .position = { 1.0f,  -1.0f, 0.0f }, .uv = { 1.0f, 1.0f } },  // bottom right
        { .position = { 1.0f,  1.0f, 0.0f },  .uv = { 1.0f, 0.0f } } // top right
    };

    constexpr static uint32_t kScreenQuadIndices[] = {
        0, 1, 2,
        1, 3, 2
    };
    
    constexpr static auto kInput = std::to_array<rhi::InputElement>({
        { "POSITION", rhi::Format::float32x3, offsetof(assets::Vertex, position) },
        { "TEXCOORD", rhi::Format::float32x2 , offsetof(assets::Vertex, uv) }
    });

    constexpr static auto kSamplers = std::to_array<rhi::Sampler>({
        { rhi::ShaderVisibility::ePixel }
    });

    constexpr static auto kRanges = std::to_array<rhi::BindingRange>({
        { 0, rhi::Object::eTexture, rhi::BindingMutability::eStaticAtExecute }
    });

    constexpr static auto kBindings = std::to_array<rhi::Binding>({
        rhi::bindTable(rhi::ShaderVisibility::ePixel, kRanges)
    });
};

struct ImGuiPass final : Pass {
    ImGuiPass(const Info& info, ImGuiUpdate update) 
        : Pass(info, CommandSlot::ePost) 
        , update(update) 
    {
        renderTargetIn = newInput<Input>("rtv", State::eRenderTarget);
        renderTargetOut = newOutput<Relay>("rtv", renderTargetIn);
    }
    
    void init(Context& ctx) override {
        imguiHeapOffset = ctx.getHeap().alloc(DescriptorSlot::eImGui);
        ctx.imguiInit(imguiHeapOffset);
    }

    void deinit(Context& ctx) override {
        ctx.imguiShutdown();
        ctx.getHeap().release(DescriptorSlot::eImGui, imguiHeapOffset);
    }

    void execute(Context& ctx) override {
        auto& cmd = ctx.getCommands(getSlot());
        // imgui work

        ctx.imguiFrame();
        ImGui::NewFrame();

        update();

        for (auto& [name, pass] : getParent()->passes) {
            pass->imgui(ctx);
        }

        cmd.imguiFrame();
    }

    WireHandle<Input, RenderTargetResource> renderTargetIn;
    Output *renderTargetOut;

private:
    size_t imguiHeapOffset;
    ImGuiUpdate update;
};

WorldGraph::WorldGraph(const WorldGraphInfo& info) : Graph(info.ctx) {
    auto global = addPass<GlobalPass>("global");
    auto scene = addPass<ScenePass>("scene");
    auto post = addPass<PostPass>("post");
    auto imgui = addPass<ImGuiPass>("imgui", info.update);
    auto present = addPass<PresentPass>("present");

    // post.renderTarget <= global.renderTarget
    // scene.sceneTarget <= global.sceneTarget
    link(post->renderTargetIn, global->renderTargetOut);
    link(scene->sceneTargetIn, global->sceneTargetOut);

    // post.sceneTarget <= scene.sceneTarget
    link(post->sceneTargetIn, scene->sceneTargetOut);

    // imgui.renderTarget <= post.renderTarget
    link(imgui->renderTargetIn, post->renderTargetOut);

    // present.renderTarget <= imgui.renderTarget
    // present.sceneTarget <= scene.sceneTarget
    link(present->renderTargetIn, imgui->renderTargetOut);
    link(present->sceneTargetIn, post->sceneTargetOut);

    this->primary = present;
    this->scene = scene;
}

void WorldGraph::addWorld(const char* path) {
    auto *world = static_cast<ScenePass*>(scene);
    world->addWorld(path);
}