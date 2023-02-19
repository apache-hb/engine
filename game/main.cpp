#include "game/gdk.h"

#include "game/game.h"
#include "game/render.h"
#include "game/window.h"

#include "simcoe/simcoe.h"

#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_win32.h"

#include <filesystem>

#include <fastgltf/fastgltf_parser.hpp>
#include <fastgltf/fastgltf_types.hpp>

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

    detail.windowResolution = window.size();
    detail.renderResolution = { 800, 600 };

    render::Context context { window, { } };
    game::Scene scene { context, detail, input.manager };

    ImGui_ImplWin32_Init(window.getHandle());
    ImGui_ImplWin32_EnableDpiAwareness();

    scene.start();

    while (window.poll()) {
        input.poll();
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
