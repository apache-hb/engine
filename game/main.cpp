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

int commonMain() {
    game::Info detail;

    simcoe::addSink(&detail.sink);
    gLog.info("cwd: {}", std::filesystem::current_path().string());

    system::System system;
    ImGuiRuntime imgui;
    game::gdk::Runtime gdk;
    game::Input input;

    game::Window window { input.mouse, input.keyboard };

    game::FirstPerson camera { { 0, 0, 50 }, { 0, 10, 50 }, 90.f };

    detail.windowResolution = window.size();
    detail.renderResolution = { 1920, 1080 };
    detail.pCamera = &camera;

    render::Context context { window, { } };
    game::Scene scene { context, detail, input.manager };

    ImGui_ImplWin32_Init(window.getHandle());
    ImGui_ImplWin32_EnableDpiAwareness();

    auto debug = game::debug.newEntry([&] {
        if (ImGui::Begin("Camera")) {
            ImGui::SliderFloat("FOV", &camera.fov, 45.f, 120.f);
            ImGui::InputFloat3("Position", &camera.position.x);
            ImGui::InputFloat3("Direction", &camera.direction.x);
        }
        ImGui::End();
    });

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
