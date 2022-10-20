#include "engine/base/macros.h"
#include "engine/base/io/io.h"
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
using namespace simcoe::input;

constexpr float kMoveSensitivity = 0.001f;
constexpr float kLookSensitivity = 0.001f;

struct CameraListener final : input::Listener {
    float speedMultiplier { 1.f };
    CameraListener(render::Perspective& camera, Keyboard& keyboard) 
        : camera(camera) 
        , keyboard(keyboard)
    { }

    bool update(const input::State& input) override {
        state = input;

        if (console.update(input.key[Key::keyTilde])) {
            logging::get(logging::eInput).info("console: {}", console.get());

            keyboard.captureInput(!console.get());
        }

        // imgui should capture input if the console is open
        ImGui::SetNextFrameWantCaptureKeyboard(console.get());
        ImGui::SetNextFrameWantCaptureMouse(console.get());
        
        if (console.get()) {
            return true;
        }

        float x = getAxis(input, Key::keyA, Key::keyD);
        float y = getAxis(input, Key::keyS, Key::keyW);
        float z = getAxis(input, Key::keyQ, Key::keyE);

        math::float3 offset = {
            .x = (x + state.axis[Axis::padLeftStickHorizontal]) * kMoveSensitivity * speedMultiplier,
            .y = (y + state.axis[Axis::padLeftStickVertical]) * kMoveSensitivity * speedMultiplier,
            .z = (z + (state.axis[Axis::padRightTrigger] - state.axis[Axis::padLeftTrigger])) * kMoveSensitivity * speedMultiplier
        };

        float yaw = (state.axis[Axis::mouseHorizontal] + state.axis[Axis::padRightStickHorizontal]) * kLookSensitivity;
        float pitch = (state.axis[Axis::mouseVertical] + state.axis[Axis::padRightStickVertical]) * kLookSensitivity;

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

            ImGui::SliderFloat("Speed", &speedMultiplier, 0.1f, 10.f);

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
    float getAxis(const input::State& input, Key::Slot negative, Key::Slot positive) {
        if (!input.key[negative] && !input.key[positive]) {
            return 0.f;
        }

        return (input.key[negative] > input.key[positive]) ? -1.f : 1.f;
    }

    Toggle console { false };
    Timer timer;

    render::Perspective& camera;
    input::Keyboard &keyboard;

    input::State state { };
};

int commonMain() {
    win32::init();  
    logging::init();

    locale::Locale *english = locale::load(locale::eEnglish, ".\\resources\\locale\\english.txt");
    locale::Locale *polish = locale::load(locale::ePolish, ".\\resources\\locale\\polish.txt");

    locale::Locale *locales[] = { english, polish };

    // make a fullscreen borderless window on the primary monitor
    int width = GetSystemMetrics(SM_CXSCREEN);
    int height = GetSystemMetrics(SM_CYSCREEN);

    UniquePtr<Window> window = ui::init({
        .title = "game",
        .size = { width, height },
        .imgui = "game.ini"
    });
    
    render::Perspective camera { { 1.f, 1.f, 1.f }, { 1.f, 1.f, 1.f }, 110.f };
    auto world = assets::loadGltf("D:\\assets\\amongus\\amongus\\amongus.gltf");

    render::BasicScene scene { { &camera, &world } };

    render::Context render { { window.get(), &scene } };

    input::Keyboard keyboard { true };
    input::Gamepad gamepad { 0 };

    CameraListener state { camera, keyboard };

    input::Manager manager { { &keyboard, &gamepad }, { &state } };

    while (window->poll(&keyboard)) {
        manager.poll();

        render.begin();
        window->imgui();

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
            const char *localeNames[] = { locale::get("locale.english").data(), locale::get("locale.polish").data() };
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
