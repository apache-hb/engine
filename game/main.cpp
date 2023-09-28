#include "game/camera.h"
#include "microsoft/gdk.h"

#include "game/game.h"
#include "game/registry.h"
#include "game/render.h"
#include "game/window.h"

#include "simcoe/math/math.h"
#include "simcoe/simcoe.h"

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

    void update(system::Window& window) {
        input.mouse.captureInput(!console);
        window.hideCursor(!console);
    }

    game::ICamera *getCamera() { return &camera; }

private:
    constexpr float getAxis(const input::State& state, Key low, Key high) {
        if (state.key[low] == 0 && state.key[high] == 0) { return 0.f; }

        return state.key[low] > state.key[high] ? -1.f : 1.f;
    }

    system::Timer timer;
    input::Toggle console = false;

    game::FirstPerson camera;
    game::Input& input;

    std::unique_ptr<util::Entry> debug;
};

int commonMain() {
    game::Info detail = {
        .assets = { "build\\game\\libgame.a.p" }
    };

    simcoe::addSink(&detail.sink);
    gLog.info("cwd: {}", std::filesystem::current_path().string());

    system::System system;
    ImGuiRuntime imgui;
    vendor::microsoft::gdk::Runtime gdk;
    Camera camera { detail.input, { 0, 0, 50 }, 90.f };

    game::Window window { detail.input.mouse, detail.input.keyboard };
    window.hideCursor(false);

    detail.windowResolution = window.size();
    detail.renderResolution = { 1920, 1080 };
    detail.pCamera = camera.getCamera();

    render::Context context { window, { } };
    game::Scene scene { context, detail };

    ImGui_ImplWin32_Init(window.getHandle());
    ImGui_ImplWin32_EnableDpiAwareness();

    scene.start();

    while (window.poll()) {
        detail.input.poll();
        scene.execute();
    }

    scene.stop();

    ImGui_ImplWin32_Shutdown();

    gLog.info("bye bye");

    return 0;
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    return commonMain();
}

int main(int, const char **) {
    return commonMain();
}
