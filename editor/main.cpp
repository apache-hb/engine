#include "engine/base/macros.h"
#include "engine/base/io.h"
#include "engine/base/logging.h"
#include "engine/base/win32.h"
#include "engine/base/util.h"

#include "engine/container/unique.h"

#include "engine/input/input.h"

#include "engine/render/render.h"
#include "engine/render/scene.h"
#include "engine/render/world.h"

#include "engine/ui/ui.h"

#include "imgui.h"

#include "editor/widgets/file-dialog.h"

#include <fstream>

using namespace simcoe;
using namespace math;

constexpr float kMoveSensitivity = 0.001f;
constexpr float kLookSensitivity = 0.001f;

namespace {
    constexpr auto kDockFlags = ImGuiDockNodeFlags_PassthruCentralNode;

    constexpr auto kWindowFlags = 
        ImGuiWindowFlags_MenuBar |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoBackground |
        ImGuiWindowFlags_NoBringToFrontOnFocus | 
        ImGuiWindowFlags_NoNavFocus;

    ImGui::FileBrowser browser;    
    std::string filename;
}

namespace editor {
    void dock() {
        const auto *viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));

        ImGui::Begin("Editor", nullptr, kWindowFlags);

        ImGui::PopStyleVar(3);

        ImGuiID id = ImGui::GetID("EditorDock");
        ImGui::DockSpace(id, ImVec2(0.f, 0.f), kDockFlags);
        
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("Import")) {
                    browser.SetTitle("Import Scene");
                    browser.SetTypeFilters({ ".gltf", ".glb" });

                    browser.Open();
                }
                ImGui::MenuItem("Save");

                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }

        ImGui::End();

        browser.Display();

        if (browser.HasSelected()) {
            filename = browser.GetSelected().string();
        }
    }
}

struct CameraListener final : input::Listener {
    bool update(const input::State& state) override {
        device = state.source;
        return true;
    }

    bool enableConsole() const {
        return false;
    }

    input::Device getDevice() const {
        return device;
    }

private:
    input::Device device = input::Device::eDesktop;
};

int commonMain() {
    win32::init();  
    logging::init();

    auto world = assets::loadGltf("B:\\assets\\deccer-cubes-main\\SM_Deccer_Cubes_Textured.gltf");

    // make a fullscreen borderless window on the primary monitor
    int width = GetSystemMetrics(SM_CXSCREEN);
    int height = GetSystemMetrics(SM_CYSCREEN);

    UniquePtr<Window> window = ui::init({
        .title = "editor",
        .size = { width, height },
        .imgui = "editor.ini"
    });
    render::Perspective camera { { 1.f, 1.f, 1.f }, { 0.f, 0.f, 0.f }, 110.f };

    render::BasicScene scene { { &camera, &world } };

    render::Context render { { window.get(), &scene } };

    Timer timer;
    input::Keyboard keyboard { };
    input::Gamepad gamepad { 0 };

    CameraListener state { };

    input::Manager manager { { &keyboard, &gamepad }, { &state } };

    while (window->poll()) {
        manager.poll();

        render.begin();
        window->imguiNewFrame();

        ImGui::NewFrame();
        ImGui::ShowDemoWindow();

        if (ImGui::Begin("Input")) {
            ImGui::Text("Method: %s", state.getDevice() == input::Device::eGamepad ? "Controller" : "Mouse & Keyboard");
        }
        ImGui::End();

        if (ImGui::Begin("Camera")) {
            auto mvp = camera.mvp(float4x4::identity(), window->size().aspectRatio<float>());
            ImGui::Text("%f %f %f %f", mvp.at(0, 0), mvp.at(0, 1), mvp.at(0, 2), mvp.at(0, 3));
            ImGui::Text("%f %f %f %f", mvp.at(1, 0), mvp.at(1, 1), mvp.at(1, 2), mvp.at(1, 3));
            ImGui::Text("%f %f %f %f", mvp.at(2, 0), mvp.at(2, 1), mvp.at(2, 2), mvp.at(2, 3));
            ImGui::Text("%f %f %f %f", mvp.at(3, 0), mvp.at(3, 1), mvp.at(3, 2), mvp.at(3, 3));
        }
        ImGui::End();

        ImGui::Render();

        render.end();
    }
    return 0;
}

int WINAPI WinMain(UNUSED HINSTANCE hInstance, UNUSED HINSTANCE hPrevInstance, UNUSED LPSTR CmdLine, UNUSED int nCmdShow) {
    return commonMain();
}

int main(UNUSED int argc, UNUSED const char **argv) {
    return commonMain();
}
