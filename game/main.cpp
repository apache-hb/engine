#include "engine/base/macros.h"
#include "engine/base/io.h"
#include "engine/base/logging.h"
#include "engine/base/win32.h"
#include "engine/base/util.h"

#include "engine/input/input.h"

#include "engine/render/render.h"
#include "engine/render/scene.h"
#include "engine/render/world.h"

#include "engine/ui/ui.h"

#include "imgui.h"

using namespace simcoe;
using namespace math;

constexpr float kMoveSensitivity = 0.001f;
constexpr float kLookSensitivity = 0.001f;

int commonMain() {
    win32::init();  
    logging::init();

    // make a fullscreen borderless window on the primary monitor
    UNUSED int width = GetSystemMetrics(SM_CXSCREEN);
    UNUSED int height = GetSystemMetrics(SM_CYSCREEN);

    UniquePtr<Window> window = ui::init({
        .title = "game",
        .size = { 800, 600 },
        .imgui = "game.ini"
    });
    
    render::Perspective camera { { 1.f, 1.f, 1.f }, { 0.f, 0.f, 0.f }, 110.f };
    auto world = assets::loadGltf("B:\\assets\\deccer-cubes-main\\SM_Deccer_Cubes_Textured.gltf");

    render::BasicScene scene { { &camera, &world } };

    render::Context render { { window.get(), &scene } };

    Timer timer;
    input::Gamepad gamepad { 0 };
    input::Input state = {
        .enableConsole = false
    };
    // float total = 0.f;

    while (window->poll(&state)) {
        gamepad.poll(&state);
        // total += float(timer.tick());

        // camera.setPosition({ std::sin(total), std::cos(total), 1.f });

        if (state.enableConsole && state.device == input::eDesktop) {
            state.rotation = { 0.f, 0.f };
        }

        camera.move(state.movement * kMoveSensitivity);
        camera.rotate(state.rotation.x * kLookSensitivity, state.rotation.y * kLookSensitivity);

        render.begin();
        window->imguiNewFrame();

        ImGui::NewFrame();
        ImGui::ShowDemoWindow();

        if (ImGui::Begin("Input")) {
            ImGui::Text("Method: %s", state.device == input::eGamepad ? "Controller" : "Mouse & Keyboard");
            ImGui::Text("Movement %.2f %.2f %.2f", state.movement.x, state.movement.y, state.movement.z);
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

    logging::get(logging::eGeneral).info("clean exit");

    return 0;
}

int WINAPI WinMain(UNUSED HINSTANCE hInstance, UNUSED HINSTANCE hPrevInstance, UNUSED LPSTR CmdLine, UNUSED int nCmdShow) {
    return commonMain();
}

int main(UNUSED int argc, UNUSED const char **argv) {
    return commonMain();
}
