#include "engine/render-new/scene.h"

#include "engine/rhi/rhi.h"
#include "imgui/imgui.h"

#include <array>

using namespace simcoe;
using namespace simcoe::render;

struct RenderTargetResource final : Resource {
    RenderTargetResource(const Info& info)
        : Resource(info)
    { }

    void addBarrier(Barriers& barriers, Output* output, Input* input) override {
        Resource::addBarrier(barriers, output, input);

        auto before = output->getState();
        auto after = input->getState();

        if (before == after) { return; }
        if (before == State::eInvalid || after == State::eInvalid) { return; }

        barriers.push_back(rhi::newStateTransition(getContext().getRenderTarget(), before, after));
    }
};

struct TextureResource final : Resource {
    TextureResource(const Info& info, rhi::TextureSize size, State initial, bool hostVisible) : Resource(info) { 
        auto& ctx = getContext();
        auto& device = ctx.getDevice();

        const auto desc = rhi::newTextureDesc(size, initial);
        const auto visibility = hostVisible ? rhi::DescriptorSet::Visibility::eHostVisible : rhi::DescriptorSet::Visibility::eDeviceOnly;

        buffer = device.newTexture(desc, visibility);
    }

    void addBarrier(Barriers& barriers, Output* output, Input* input) override {
        Resource::addBarrier(barriers, output, input);

        auto before = output->getState();
        auto after = input->getState();

        if (before == after) { return; }
        if (before == State::eInvalid || after == State::eInvalid) { return; }

        barriers.push_back(rhi::newStateTransition(buffer, before, after));
    }

    rhi::Buffer& get() { return buffer; }

private:
    rhi::Buffer buffer;
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
        renderTargetIn = newInput<Input>("rtv", State::ePresent);
    }

    void execute(Context& ctx) override {
        ctx.endFrame();
        ctx.present();
    }
    
    Input *renderTargetIn;
};

struct ScenePass final : Pass {
    ScenePass(const Info& info) : Pass(info) {
        auto& ctx = getContext();
        sceneTargetResource = getParent()->addResource<TextureResource>("scene-target", ctx.sceneSize(), State::eRenderTarget, false);
        sceneTargetOut = newOutput<Source>("scene-target", State::eRenderTarget, sceneTargetResource);
    }

    void execute(Context&) override {
        
    }

    TextureResource *sceneTargetResource;
    Output *sceneTargetOut;
};

struct PostPass final : Pass {
    PostPass(const Info& info) : Pass(info) {
        sceneTargetIn = newInput<Input>("scene-target", State::ePixelShaderResource);
        renderTargetOut = newOutput<Output>("rtv", State::eRenderTarget);
    }

    void init(Context& ctx) override {
        initView(ctx);
        initScreenQuad(ctx);
    }

    void execute(Context& ctx) override {
        auto& cmd = ctx.getCommands();
        auto& heap = ctx.getHeap();
        auto kHeaps = std::to_array({ heap.get() });

        cmd.setViewAndScissor(view);
        cmd.bindDescriptors(kHeaps);
        cmd.setPipeline(pso);
    }

    Input *sceneTargetIn;

    Output *renderTargetOut;

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
        auto ps = loadShader("resources/shaders/post-shader.ps.pso");
        auto vs = loadShader("resources/shaders/post-shader.vs.pso");

        pso = device.newPipelineState({
            .samplers = kSamplers,
            .bindings = kBindings,
            .input = kInput,
            .ps = ps,
            .vs = vs
        });
        // TODO: upload quad
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
    
    constexpr static math::float4 kLetterBox = { 0.f, 0.f, 0.f, 1.f };

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
        auto kDescriptors = std::to_array({ ctx.getHeap().get() });

        cmd.bindDescriptors(kDescriptors);
        cmd.setRenderTarget(ctx.getRenderTargetHandle(), rhi::CpuHandle::Invalid, kLetterBox);
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

    Input *renderTargetIn;
    Output *renderTargetOut;
};

WorldGraph::WorldGraph(Context& ctx) : Graph(ctx) {
    auto globalPass = addPass<GlobalPass>("global");
    auto imgui = addPass<ImGuiPass>("imgui");
    auto present = addPass<PresentPass>("present");

    link(imgui->renderTargetIn, globalPass->renderTargetOut);
    link(present->renderTargetIn, imgui->renderTargetOut);

    primary = present;
}
