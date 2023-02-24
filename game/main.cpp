#include "game/camera.h"
#include "game/gdk.h"

#include "game/game.h"
#include "game/registry.h"
#include "game/render.h"
#include "game/window.h"

#include "simcoe/simcoe.h"

#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_win32.h"

#include <filesystem>

using namespace simcoe;
using namespace simcoe::input;

struct ImGuiRuntime {
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
        debug = game::debug.newEntry([&] {
            if (ImGui::Begin("Camera")) {
                ImGui::SliderFloat("FOV", &camera.fov, 45.f, 120.f);
                ImGui::InputFloat3("Position", &camera.position.x);
                ImGui::InputFloat3("Direction", &camera.direction.x);
            }
            ImGui::End();
        });

        game.add(this);
    }

    void accept(const input::State& state) override {
        if (console.update(state.key[Key::keyTilde])) {
            gInputLog.info("console: {}", console ? "enabled" : "disabled");
            input.mouse.captureInput(!console);
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

    game::ICamera *getCamera() { return &camera; }

private:
    float getAxis(const input::State& state, Key low, Key high) {
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
    game::Info detail;

    simcoe::addSink(&detail.sink);
    gLog.info("cwd: {}", std::filesystem::current_path().string());

    system::System system;
    ImGuiRuntime imgui;
    game::gdk::Runtime gdk;
    game::Input input;
    Camera camera { input, { 0, 0, 50 }, 90.f };

    game::Window window { input.mouse, input.keyboard };

    detail.windowResolution = window.size();
    detail.renderResolution = { 1920, 1080 };
    detail.pCamera = camera.getCamera();

    render::Context context { window, { } };
    game::Scene scene { context, detail, input.manager };

    ImGui_ImplWin32_Init(window.getHandle());
    ImGui_ImplWin32_EnableDpiAwareness();

    scene.start();

    while (window.poll()) {
        input.poll();
        scene.execute();

        // TODO: move camera
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
