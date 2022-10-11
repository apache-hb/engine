#include "engine/base/macros.h"
#include "engine/base/io.h"
#include "engine/base/logging.h"
#include "engine/window/window.h"
#include "engine/base/win32.h"
#include "engine/base/util.h"

#include "engine/input/input.h"

#include "engine/render/render.h"
#include "engine/render/scene.h"

#include "imgui.h"

using namespace engine;
using namespace math;

int commonMain() {
    win32::init();

    // setup imgui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    auto& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();

    UniquePtr<Io> logFile { Io::open("game.log", Io::eWrite) };
    logging::IoChannel fileLogger {"general", logFile.get(), logging::eFatal};
    logging::ConsoleChannel consoleLogger { "general", logging::eDebug};

    logging::Channel *channels[] { &fileLogger, &consoleLogger };
    logging::MultiChannel logger { "general", channels };

    // make a fullscreen borderless window on the primary monitor
    int width = GetSystemMetrics(SM_CXSCREEN);
    int height = GetSystemMetrics(SM_CYSCREEN);

    Window window { width, height, "game" };
    render::LookAt camera { { 1.f, 1.f, 1.f }, { 0.0f, 0.f, 0.f }, 110.f };

    render::BasicScene scene { { camera } };

    render::Context render { { &window, &logger, scene } };

    Timer timer;
    float total = 0.f;

    while (window.poll()) {
        UNUSED auto input = input::poll();
        total += float(timer.tick());

        camera.setPosition({ std::sin(total), std::cos(total), 1.f });

        render.begin();
        window.imguiNewFrame();
        ImGui::NewFrame();

        ImGui::ShowDemoWindow();

        ImGui::Render();

        render.end();
    }

    logger.info("clean exit");

    return 0;
}

int WINAPI WinMain(UNUSED HINSTANCE hInstance, UNUSED HINSTANCE hPrevInstance, UNUSED LPSTR CmdLine, UNUSED int nCmdShow) {
    return commonMain();
}

int main(UNUSED int argc, UNUSED const char **argv) {
    return commonMain();
}
