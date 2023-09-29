#include "game/camera.h"
#include "game/game.h"
#include "game/registry.h"
#include "game/render.h"

#include "microsoft/gdk.h"

#include "simcoe/math/math.h"
#include "simcoe/simcoe.h"

#include "simcoe/rhi/rhi.h"

#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_win32.h"

#include <filesystem>

using namespace simcoe;
using namespace simcoe::input;

struct ImGuiRuntime final {
    ImGuiRuntime() {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui::StyleColorsDark();

        auto& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    }

    ~ImGuiRuntime() {
        ImGui::DestroyContext();
    }
};

struct Camera final : input::ITarget {
    Camera(game::Input& game, math::float3 position, float fov)
        : camera(position, { 0, 0, 1 }, fov)
        , input(game)
    {
        debug = game::debug.newEntry({ "Camera" }, [&] {
            ImGui::SliderFloat("FOV", &camera.fov, 45.f, 120.f);
            ImGui::InputFloat3("Position", &camera.position.x);
            ImGui::InputFloat3("Direction", &camera.direction.x);
        });

        game.add(this);
    }

    void accept(const input::State& state) override {
        if (console.update(state.key[Key::keyTilde])) {
            gInputLog.info("console: {}", console ? "enabled" : "disabled");
        }

        ImGui::SetNextFrameWantCaptureKeyboard(console);
        ImGui::SetNextFrameWantCaptureMouse(console);

        if (console) { return; }

        float x = getAxis(state, Key::keyA, Key::keyD);
        float y = getAxis(state, Key::keyS, Key::keyW);
        float z = getAxis(state, Key::keyQ, Key::keyE);

        float yaw = state.axis[Axis::mouseHorizontal] * 0.01f;
        float pitch = -state.axis[Axis::mouseVertical] * 0.01f;

        camera.move({ x, y, z });
        camera.rotate(yaw, pitch);
    }

    void update(os::Window& window) {
        input.mouse.captureInput(!console);
        window.hideCursor(!console);
    }

    game::ICamera *getCamera() { return &camera; }

private:
    constexpr float getAxis(const input::State& state, Key low, Key high) {
        if (state.key[low] == 0 && state.key[high] == 0) { return 0.f; }

        return state.key[low] > state.key[high] ? -1.f : 1.f;
    }

    os::Timer timer;
    input::Toggle console = false;

    game::FirstPerson camera;
    game::Input& input;

    std::unique_ptr<util::Entry> debug;
};

#if 0
rhi::IContext *getRenderLibrary(const char *path) {
    HMODULE hModule = LoadLibrary(path);
    if (!hModule) {
        PANIC("failed to load render library: {}", path);
    }

    rhi::CreateContext pCreateContext = (rhi::CreateContext)GetProcAddress(hModule, "rhiGetContext");
    if (!pCreateContext) {
        PANIC("failed to find rhiGetContext in render library: {}", path);
    }

    return (rhi::IContext*)pCreateContext();
}

struct RenderContext {
    static constexpr size_t kFrames = 2;
    static constexpr math::float4 kClear = { 0.0f, 0.2f, 0.4f, 1.0f };

    RenderContext(rhi::IContext *ctx, os::Window& window) : pContext(ctx) {
        getAdapters();

        for (size_t i = 0; i < adapters.size(); i++) {
            auto pAdapter = adapters[i];
            auto info = pAdapter->getInfo();

            gLog.info("adapter #{}: {}{}", i, info.name, info.type == rhi::AdapterType::eSoftware ? " (software)" : "");
        }

        auto* pAdapter = adapters[0];
        pDevice = pAdapter->createDevice();

        pQueue = pDevice->createCommandQueue(rhi::CommandType::eDirect);

        pDisplay = pQueue->createDisplayQueue(pContext, {
            .window = window,
            .size = { 1920, 1080 },
            .format = rhi::ColourFormat::eRGBA8,
            .frames = kFrames
        });

        pRenderTargetHeap = pDevice->createHeap(rhi::HeapType::eRenderTarget, kFrames);

        for (size_t i = 0; i < kFrames; i++) {
            pRenderTarget[i] = pDisplay->getSurface(i);
            pDevice->mapRenderTarget(pRenderTarget[i], pRenderTargetHeap->getCpuHandle(i));
        }

        pCommands = pDevice->createCommandList(rhi::CommandType::eDirect);
        pFence = pDevice->createFence();
    }

    void present() {
        beginFrame();

        rhi::ICommandList *pLists[] = { pCommands };

        pQueue->execute(pLists);
        pDisplay->present();

        waitForFrame();
    }

private:
    void beginFrame() {
        frameIndex = pDisplay->getCurrentFrame();
        pCommands->begin();

        rhi::Barrier toTarget[] = { rhi::transition(pRenderTarget[frameIndex], rhi::ResourceState::ePresent, rhi::ResourceState::eRenderTarget) };
        rhi::Barrier toPresent[] = { rhi::transition(pRenderTarget[frameIndex], rhi::ResourceState::eRenderTarget, rhi::ResourceState::ePresent) };

        pCommands->transition(toTarget);
        pCommands->clearRenderTarget(pRenderTargetHeap->getCpuHandle(frameIndex), kClear);
        pCommands->transition(toPresent);

        pCommands->end();
    }

    void waitForFrame() {
        size_t value = fenceValue++;
        pQueue->signal(pFence, value);

        if (pFence->getValue() < value) {
            pFence->wait(value);
        }
    }

    void getAdapters() {
        while (true) {
            rhi::IAdapter *pAdapter = pContext->getAdapter(adapters.size());
            if (!pAdapter) { break; }

            adapters.push_back(pAdapter);
        }
    }

    rhi::IContext *pContext;
    std::vector<rhi::IAdapter*> adapters;
    rhi::IDevice *pDevice;

    rhi::IDisplayQueue *pDisplay;

    rhi::ICommandQueue *pQueue;
    rhi::ICommandList *pCommands;

    rhi::IHeap *pRenderTargetHeap;
    rhi::ISurface *pRenderTarget[kFrames];

    size_t frameIndex = 0;
    rhi::IFence *pFence;
    size_t fenceValue = 1;
};
#endif

void commonMain(os::System& system, int nCmdShow) {
    game::GuiSink guiSink{};
    simcoe::addSink(&guiSink);

    assets::Manager assets = { "build\\game\\libgame.a.p" };
    input::Mouse mouseInput = { false, true };
    input::Keyboard keyboardInput;
    game::GameEvents events = { keyboardInput };

    const os::WindowInfo info = {
        .title = "Game",
        .size = { 1920, 1080 },
        .style = os::WindowStyle::eBorderless,
        .pEvents = &events
    };

    auto window = system.openWindow(nCmdShow, info);
    window.hideCursor(false);

    game::Input input { keyboardInput, mouseInput };

#if 0
    ASSERT(SetDllDirectoryA("build") != 0);

    RenderContext ctx{getRenderLibrary("rhi-d3d12.dll"), window};

    while (system.poll()) {
        mouseInput.update(window.getHandle());
        ctx.present();
    }
#endif

    ImGuiRuntime imgui;
    Camera camera { input, { 0, 0, 50 }, 90.f };

    game::Info detail = {
        .windowResolution = window.getSize(),
        .renderResolution = { 1920, 1080 },

        .input = input,
        .assets = assets,

        .sink = guiSink,
        .pCamera = camera.getCamera()
    };

    render::Context context { window, { } };
    game::Scene scene { context, detail };

    ImGui_ImplWin32_Init(window.getHandle());
    ImGui_ImplWin32_EnableDpiAwareness();

    scene.start();

    while (system.poll()) {
        mouseInput.update(window.getHandle());
        detail.input.poll();
        scene.execute();
    }

    scene.stop();

    ImGui_ImplWin32_Shutdown();
}

int outerMain(os::System& system, int nCmdShow) {
    gLog.info("cwd: {}", std::filesystem::current_path().string());

    commonMain(system, nCmdShow);

    gLog.info("exiting main");
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    os::System system = { hInstance };
    return outerMain(system, nCmdShow);
}

int main(int, const char **) {
    os::System system = { GetModuleHandle(nullptr) };
    return outerMain(system, SW_SHOWDEFAULT);
}
