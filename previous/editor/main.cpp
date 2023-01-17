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
#include "engine/render/assets/assets.h"

#include "engine/ui/ui.h"

#include "imgui.h"

#include "editor/widgets/file-dialog.h"

#include <fstream>

using namespace simcoe;
using namespace simcoe::math;
using namespace simcoe::input;

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
    float speedMultiplier { 1.f };
    CameraListener(render::Perspective& camera, Keyboard& keyboard) 
        : camera(camera) 
        , console(!keyboard.isEnabled())
        , keyboard(keyboard)
    { }

    bool update(const input::State& input) override {
        state = input;

        if (console.update(input.key[Key::keyTilde])) {
            logging::v2::info(logging::eInput, "console: {}", console.get());

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

    Timer timer;

    render::Perspective& camera;
    Toggle console { true };

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
        .title = "editor",
        .size = { width, height },
        .imgui = "editor.ini"
    });

    render::Perspective camera { { 1.f, 1.f, 1.f }, { 0.f, 0.f, 1.f }, 110.f };

    render::BasicScene scene { { &camera } };

    render::Context render { { window.get(), &scene } };

    input::Keyboard keyboard { };
    input::Gamepad gamepad { 0 };

    CameraListener state { camera, keyboard };

    input::Manager manager { { &keyboard, &gamepad }, { &state } };

    assets::loadGltf(&scene, "D:\\assets\\glTF-Sample-Models-master\\2.0\\Sponza\\glTF\\Sponza.gltf");

    while (window->poll(&keyboard)) {
        manager.poll();

        render.begin();
        window->imguiFrame();

        ImGui::NewFrame();

        editor::dock();

        ImGui::ShowDemoWindow();

        state.imgui();
       
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
    return 0;
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    return commonMain();
}

int main(int, const char **) {
    return commonMain();
}
