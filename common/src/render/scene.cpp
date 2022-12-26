#include "engine/render/scene.h"
#include "engine/render/render.h"

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

        renderTargetOut = newOutput<Source>("rtv", State::ePresent, rtvResource);
    }

    void execute() override { }

    void init() override {
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

    void deinit() override {
        ImNodes::DestroyContext();
    }

    void imgui() override {
        auto& ctx = getContext();
        if (ImGui::Begin("Context state")) {
            ImGui::Text("Total frames: %zu", ctx.getFrames());

            auto [wx, wy] = ctx.windowSize();
            ImGui::Text("Present resolution: %zu x %zu", wx, wy);

            auto [sx, sy] = ctx.sceneSize();
            ImGui::Text("Scene resolution: %zu x %zu", sx, sy);

            auto& heap = ctx.getHeap();
            ImGui::Text("CBV Heap: %zu/%zu", heap.getUsed(), heap.getSize());

            if (ImGui::BeginTable("Slots", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoHostExtendX)) {
                ImGui::TableSetupColumn("Index");
                ImGui::TableSetupColumn("Type");
                ImGui::TableHeadersRow();

                for (size_t i = 0; i < heap.getUsed(); i++) {
                    ImGui::TableNextRow();

                    ImGui::TableNextColumn();
                    ImGui::Text("%zu", i);

                    ImGui::TableNextColumn();
                    ImGui::Text("%s", getSlotName(heap.getBit(i)));
                }

                ImGui::EndTable();
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
    Output *renderTargetOut;

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

    void execute() override {
        auto& ctx = getContext();
        ctx.endFrame();
        ctx.present();
    }
    
    Input *renderTargetIn;
    Input *sceneTargetIn;
};

struct AsyncDraw {
    using Action = std::function<void()>;
    void add(Action action) {
        std::lock_guard lock(mutex);
        commands.push_back(std::move(action));
    }

    void apply() {
        std::lock_guard lock(mutex);
        for (auto& command : commands) {
            command();
        }
    }
private:
    std::mutex mutex;
    std::vector<Action> commands;
};

struct ScenePass final : Pass {
    ScenePass(const Info& info) : Pass(info, CommandSlot::eScene) {
        sceneTargetResource = getParent()->addResource<SceneTargetResource>("scene-target", State::eRenderTarget);

        sceneTargetOut = newOutput<Source>("scene-target", State::eRenderTarget, sceneTargetResource);
    }

    void init() override { }

    void execute() override {
        auto& ctx = getContext();
        auto& cmd = getCommands();

        auto [width, height] = ctx.sceneSize().as<float>();

        cmd.setViewAndScissor(rhi::View(0, 0, width, height));
        cmd.clearRenderTarget(sceneTargetResource->rtvHandle(), kClearColour);
    }

    void imgui() override {
        if (ImGui::Begin("Scene")) {
            static char path[256] {};
            ImGui::InputTextWithHint("##", "gltf path", path, sizeof(path));
            
            if (ImGui::Button("Load model")) {
                // todo
            }
        }
        ImGui::End();
    }

    SceneTargetResource *sceneTargetResource;
    Output *sceneTargetOut;

private:
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
        sceneTargetOut = newOutput<Relay>("scene-target", sceneTargetIn); 
        
        renderTargetIn = newInput<Input>("rtv", State::eRenderTarget);
        renderTargetOut = newOutput<Relay>("rtv", renderTargetIn);
    }

    void init() override {
        initView();
        initScreenQuad();
    }

    void execute() override {
        auto& cmd = getCommands();
        auto *renderTarget = renderTargetIn.get();

        cmd.setViewAndScissor(view);
        cmd.setRenderTarget(renderTarget->rtvCpuHandle(), rhi::CpuHandle::Invalid, kLetterBox);
        
        draws.apply();
    }

    WireHandle<Input, SceneTargetResource> sceneTargetIn;
    WireHandle<Input, RenderTargetResource> renderTargetIn;

    Output *renderTargetOut;
    Output *sceneTargetOut;

private:
    void initView() {
        auto& ctx = getContext();
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

    void initScreenQuad() {
        auto& ctx = getContext();
        auto& device = ctx.getDevice();
        auto& copy = ctx.getCopyQueue();

        auto ps = loadShader("resources/shaders/post-shader.ps.pso");
        auto vs = loadShader("resources/shaders/post-shader.vs.pso");

        pso = device.newPipelineState({
            .samplers = kSamplers,
            .bindings = kBindings,
            .input = kInput,
            .ps = ps,
            .vs = vs
        });

        auto vboCopy = copy.beginDataUpload(kScreenQuad, sizeof(kScreenQuad));
        auto iboCopy = copy.beginDataUpload(kScreenQuadIndices, sizeof(kScreenQuadIndices));

        auto batchCopy = copy.beginBatchUpload({ vboCopy, iboCopy });

        copy.submit(batchCopy, [&, vboCopy, iboCopy] {
            vbo = vboCopy->getBuffer();
            ibo = iboCopy->getBuffer();

            rhi::VertexBufferView vboView = rhi::newVertexBufferView(vbo.gpuAddress(), std::size(kScreenQuad), sizeof(assets::Vertex));
            rhi::IndexBufferView iboView = {
                .buffer = ibo.gpuAddress(),
                .length = std::size(kScreenQuadIndices),
                .format = rhi::Format::uint32
            };

            draws.add([&, vboView, iboView] {
                auto& cmd = getCommands();
                auto* sceneTarget = sceneTargetIn.get();

                cmd.setPipeline(pso);
                cmd.bindTable(0, sceneTarget->cbvGpuHandle());
                cmd.setVertexBuffers(std::to_array({ vboView }));
                cmd.drawIndexed(iboView);
            });
        });
    }

    rhi::View view;

    rhi::PipelineState pso;
    rhi::Buffer vbo;
    rhi::Buffer ibo;

    AsyncDraw draws;

    constexpr static assets::Vertex kScreenQuad[] = {
        { .position = { -1.0f, -1.0f, 0.0f }, .uv = { 0.0f, 1.0f } }, // bottom left
        { .position = { -1.0f, 1.0f,  0.0f }, .uv = { 0.0f, 0.0f } },  // top left
        { .position = { 1.0f,  -1.0f, 0.0f }, .uv = { 1.0f, 1.0f } },  // bottom right
        { .position = { 1.0f,  1.0f,  0.0f }, .uv = { 1.0f, 0.0f } } // top right
    };

    constexpr static uint32_t kScreenQuadIndices[] = {
        0, 1, 2,
        1, 3, 2
    };
    
    constexpr static auto kInput = std::to_array<rhi::InputElement>({
        { "POSITION", rhi::Format::float32x3, offsetof(assets::Vertex, position) },
        { "TEXCOORD", rhi::Format::float32x2, offsetof(assets::Vertex, uv) }
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
    
    void init() override {
        auto& ctx = getContext();
        imguiHeapOffset = ctx.getHeap().alloc(DescriptorSlot::eImGui);
        ctx.imguiInit(imguiHeapOffset);
    }

    void deinit() override {
        auto& ctx = getContext();
        ctx.imguiShutdown();
        ctx.getHeap().release(DescriptorSlot::eImGui, imguiHeapOffset);
    }

    void execute() override {
        auto& ctx = getContext();
        auto& cmd = getCommands();
        // imgui work

        ctx.imguiFrame();
        ImGui::NewFrame();

        update();

        for (auto& [name, pass] : getParent()->passes) {
            pass->imgui();
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

    // post.sceneTarget <= scene.sceneTarget
    link(post->sceneTargetIn, scene->sceneTargetOut);

    // imgui.renderTarget <= post.renderTarget
    link(imgui->renderTargetIn, post->renderTargetOut);

    // present.renderTarget <= imgui.renderTarget
    // present.sceneTarget <= scene.sceneTarget
    link(present->renderTargetIn, imgui->renderTargetOut);
    link(present->sceneTargetIn, post->sceneTargetOut);

    primary = present;
}
