#include "engine/base/macros.h"
#include "engine/base/io.h"
#include "engine/base/logging.h"
#include "engine/base/window.h"
#include "engine/base/win32.h"
#include "engine/base/util.h"

#include "engine/input/input.h"

#include "engine/render/render.h"

#include <glfw/glfw3.h>

#include "imgui.h"

using namespace engine;
using namespace math;

int commonMain() {
    // we want to be dpi aware
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    
    // shut up abort
    _set_abort_behavior(0, _WRITE_ABORT_MSG);

    // setup glfw
    glfwInit();

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
    const auto *mode = glfwGetVideoMode(glfwGetPrimaryMonitor());

    Window window { mode->width, mode->height, "game" };
    render::Context render { { &window, &logger } };
    render::LookAt camera { { 1.f, 1.f, 1.f }, { 0.0f, 0.f, 0.f }, 110.f };

    Timer timer;
    float total = 0.f;

    while (window.poll()) {
        UNUSED auto input = input::poll();
        total += float(timer.tick());

        camera.setPosition({ std::sin(total), std::cos(total), 1.f });

        render.begin(&camera);
        window.imguiNewFrame();
        ImGui::NewFrame();

        ImGui::ShowDemoWindow();

        ImGui::Render();

        render.end();
    }

    glfwTerminate();

    logger.info("clean exit");

    return 0;
}

int WINAPI WinMain(UNUSED HINSTANCE hInstance, UNUSED HINSTANCE hPrevInstance, UNUSED LPSTR CmdLine, UNUSED int nCmdShow) {
    return commonMain();
}

int main(UNUSED int argc, UNUSED const char **argv) {
    return commonMain();
}