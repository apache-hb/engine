#include "engine/base/macros.h"
#include "engine/base/io.h"
#include "engine/base/logging.h"
#include "engine/base/win32.h"
#include "engine/base/util.h"

#include "engine/input/input.h"

#include "engine/locale/locale.h"
#include "engine/render/camera.h"
#include "engine/render/render.h"
#include "engine/render/scene.h"
#include "engine/render/world.h"

#include "engine/ui/ui.h"

#include "imgui.h"

using namespace simcoe;
using namespace simcoe::math;

constexpr float kMoveSensitivity = 0.001f;
constexpr float kLookSensitivity = 0.001f;

struct CameraListener final : input::Listener {
    CameraListener(render::Perspective &camera)
        : camera(camera)
    { }

    bool update(const input::State& input) override {
        state = input;

        math::float3 offset = {
            .x = state.axis[input::Axis::padLeftStickHorizontal] * kMoveSensitivity,
            .y = state.axis[input::Axis::padLeftStickVertical] * kMoveSensitivity,
            .z = (state.axis[input::Axis::padRightTrigger] - state.axis[input::Axis::padLeftTrigger]) * kMoveSensitivity
        };

        float yaw = state.axis[input::Axis::padRightStickHorizontal] * kLookSensitivity;
        float pitch = state.axis[input::Axis::padRightStickVertical] * kLookSensitivity;

        camera.rotate(yaw, pitch);
        camera.move(offset);
        
        return true;
    }

    void imgui() {
        if (ImGui::Begin("Input")) {
            auto [x, y, z] = camera.getPosition();
            auto [yaw, pitch, roll] = camera.getDirection();

            ImGui::Text("Device: %s", locale::get(state.source).data());
            ImGui::Text("Position: %f, %f, %f", x, y, z);
            ImGui::Text("Rotation: %f, %f, %f", yaw, pitch, roll);

            if (ImGui::BeginTable("Input state", 2, ImGuiTableFlags_Borders)) {
                ImGui::TableSetupColumn("Key");
                ImGui::TableSetupColumn("State");
                ImGui::TableHeadersRow();

                for (size_t id = 0; id < input::Key::eTotal; id++) {
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("%s", locale::get(input::Key::Slot(id)).data());
                    ImGui::TableNextColumn();
                    ImGui::Text("%s", state.key[id] ? "pressed" : "released");
                }

                for (size_t id = 0; id < input::Axis::eTotal; id++) {
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("%s", locale::get(input::Axis::Slot(id)).data());
                    ImGui::TableNextColumn();
                    ImGui::Text("%f", state.axis[id]);
                }

                ImGui::EndTable();
            }
        }
        ImGui::End();
    }

private:
    Timer timer;
    render::Perspective& camera;

    input::State state { };
};

int commonMain() {
    win32::init();  
    logging::init();

    locale::Locale *english = locale::load(locale::eEnglish, ".\\resources\\locale\\english.txt");
    locale::Locale *polish = locale::load(locale::ePolish, ".\\resources\\locale\\polish.txt");

    locale::Locale *locales[] = { english, polish };

    // make a fullscreen borderless window on the primary monitor
    UNUSED int width = GetSystemMetrics(SM_CXSCREEN);
    UNUSED int height = GetSystemMetrics(SM_CYSCREEN);

    UniquePtr<Window> window = ui::init({
        .title = "game",
        .size = { 800, 600 },
        .imgui = "game.ini"
    });
    
    render::Perspective camera { { 1.f, 1.f, 1.f }, { 0.f, 0.f, 1.f }, 110.f };
    auto world = assets::loadGltf("D:\\assets\\deccer-cubes-main\\SM_Deccer_Cubes_Textured.gltf");

    render::BasicScene scene { { &camera, &world } };

    render::Context render { { window.get(), &scene } };

    input::Keyboard keyboard { };
    input::Gamepad gamepad { 0 };

    CameraListener state { camera };

    input::Manager manager { { &keyboard, &gamepad }, { &state } };

    while (window->poll()) {
        manager.poll();

        render.begin();
        window->imguiNewFrame();

        ImGui::NewFrame();
        ImGui::ShowDemoWindow();

        state.imgui();

        if (ImGui::Begin("Camera")) {
            auto mvp = camera.mvp(float4x4::identity(), window->size().aspectRatio<float>());
            ImGui::Text("%f %f %f %f", mvp.at(0, 0), mvp.at(0, 1), mvp.at(0, 2), mvp.at(0, 3));
            ImGui::Text("%f %f %f %f", mvp.at(1, 0), mvp.at(1, 1), mvp.at(1, 2), mvp.at(1, 3));
            ImGui::Text("%f %f %f %f", mvp.at(2, 0), mvp.at(2, 1), mvp.at(2, 2), mvp.at(2, 3));
            ImGui::Text("%f %f %f %f", mvp.at(3, 0), mvp.at(3, 1), mvp.at(3, 2), mvp.at(3, 3));
        }
        ImGui::End();

        if (ImGui::Begin("Locale")) {
            static int locale = 0;
            const char *localeNames[] = { "English", "Polish" };
            if (ImGui::Combo("Locale", &locale, localeNames, IM_ARRAYSIZE(localeNames))) {
                locale::set(locales[locale]);
            }
            ImGui::Text("It: %s %d", localeNames[locale], locale);
            
            if (ImGui::BeginTable("Translations", 2, ImGuiTableFlags_Borders)) {
                ImGui::TableSetupColumn("Key");
                ImGui::TableSetupColumn("Text");
                ImGui::TableHeadersRow();

                for (const auto& [key, text] : locales[locale]->keys) {
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("%s", key.c_str());
                    ImGui::TableNextColumn();
                    ImGui::Text("%s", text.c_str());
                }
            }
            ImGui::EndTable();
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
