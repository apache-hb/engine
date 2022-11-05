#include "engine/render-new/scene.h"

#include "engine/rhi/rhi.h"
#include "imgui/imgui.h"

#include <array>

using namespace simcoe;
using namespace simcoe::render;

namespace {
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

        logging::get(logging::eRender).info("buffer `{}` transitioned from 0x{:x} to 0x{:x}", getName(), (unsigned)before, (unsigned)after);

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

        buffer = device.newTexture(desc, visibility, kLetterBox);
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
    GlobalPass(const Info& info) : Pass(info) {
        rtvResource = getParent()->addResource<RenderTargetResource>("rtv");
        renderTargetOut = newOutput<Source>("rtv", State::ePresent, rtvResource);
    }

    void execute(Context& ctx) override { 
        ctx.beginFrame();
    }

    RenderTargetResource *rtvResource;
    Output *renderTargetOut;
};

struct PresentPass final : Pass {
    PresentPass(const Info& info) : Pass(info) {
        sceneTargetIn = newInput<Input>("scene-target", State::eRenderTarget);
        renderTargetIn = newInput<Input>("rtv", State::ePresent);
    }

    void execute(Context& ctx) override {
        ctx.endFrame();
        ctx.present();
    }
    
    Input *sceneTargetIn;
    Input *renderTargetIn;
};

struct ScenePass final : Pass {
    ScenePass(const Info& info) : Pass(info) {
        sceneTargetResource = getParent()->addResource<SceneTargetResource>("scene-target", State::eRenderTarget);
        sceneTargetOut = newOutput<Source>("scene-target", State::eRenderTarget, sceneTargetResource);
    }

    void execute(Context&) override {
        
    }

    SceneTargetResource *sceneTargetResource;
    Output *sceneTargetOut;
};

struct PostPass final : Pass {
    PostPass(const Info& info) : Pass(info) {
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
        auto& cmd = ctx.getCommands();
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

        view.viewport.TopLeftX = windowHeight * (1.f - x) / 2.f;
        view.viewport.TopLeftY = windowHeight * (1.f - y) / 2.f;
        view.viewport.Width = x * windowWidth;
        view.viewport.Height = y * windowWidth;

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
            .length = std::size(kScreenQuad),
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
    ImGuiPass(const Info& info) : Pass(info) {
        renderTargetIn = newInput<Input>("rtv", State::eRenderTarget);
        renderTargetOut = newOutput<Relay>("rtv", renderTargetIn);
    }
    
    void init(Context& ctx) override {
        auto [width, height] = ctx.windowSize();

        view = { 0.f, 0.f, float(width), float(height) };

        imguiHeapOffset = ctx.getHeap().alloc(DescriptorSlot::eImGui);
        ctx.imguiInit(imguiHeapOffset);
    }

    void deinit(Context& ctx) override {
        ctx.imguiShutdown();
        ctx.getHeap().release(DescriptorSlot::eImGui, imguiHeapOffset);
    }

    void execute(Context& ctx) override {
        auto& cmd = ctx.getCommands();
        auto *target = renderTargetIn.get();
        cmd.setRenderTarget(target->rtvCpuHandle(), rhi::CpuHandle::Invalid, kLetterBox);
        cmd.setViewAndScissor(view);

        // imgui work

        ctx.imguiFrame();
        ImGui::NewFrame();

        ImGui::SetNextFrameWantCaptureKeyboard(true);
        ImGui::SetNextFrameWantCaptureMouse(true);
        
        ImGui::ShowDemoWindow();
        cmd.imguiFrame();
    }

    rhi::View view;

    size_t imguiHeapOffset;

    WireHandle<Input, RenderTargetResource> renderTargetIn;
    Output *renderTargetOut;
};

WorldGraph::WorldGraph(Context& ctx) : Graph(ctx) {
    auto global = addPass<GlobalPass>("global");
    auto scene = addPass<ScenePass>("scene");
    auto post = addPass<PostPass>("post");
    auto imgui = addPass<ImGuiPass>("imgui");
    auto present = addPass<PresentPass>("present");

    // global (render target) -> post -> imgui -> present
    // scene (scene target) ----^

    // post.renderTarget <= global.renderTarget
    // post.sceneTarget <= scene.sceneTarget
    link(post->renderTargetIn, global->renderTargetOut);
    link(post->sceneTargetIn, scene->sceneTargetOut);

    // imgui.renderTarget <= post.renderTarget
    link(imgui->renderTargetIn, post->renderTargetOut);

    // present.renderTarget <= imgui.renderTarget
    // present.sceneTarget <= scene.sceneTarget
    link(present->renderTargetIn, imgui->renderTargetOut);

    // TODO: this is a bit of a hack because we dont have a way to reset
    // resources to intitial state.
    link(present->sceneTargetIn, post->sceneTargetOut);
    
    primary = present;
}
