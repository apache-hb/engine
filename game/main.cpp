#include "engine/base/macros.h"
#include "engine/base/io.h"
#include "engine/base/logging.h"
#include "engine/render/render.h"
#include "engine/base/win32.h"

using namespace engine;

int commonMain() {
    // we want to be dpi aware
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    
    // shut up abort
    _set_abort_behavior(0, _WRITE_ABORT_MSG);

    // setup glfw
    glfwInit();

    std::unique_ptr<Io> logFile { Io::open("game.log", Io::eWrite) };
    logging::IoChannel fileLogger {"general", logFile.get(), logging::eFatal};
    logging::ConsoleChannel consoleLogger { "general", logging::eDebug};

    logging::Channel *channels[] { &fileLogger, &consoleLogger };
    logging::MultiChannel logger { "general", channels };

    render::enableSm6(&logger);

    // make a fullscreen borderless window on the primary monitor
    const auto *mode = glfwGetVideoMode(glfwGetPrimaryMonitor());

    Window window { mode->width, mode->height, "game" };

    render::Context render { &window, &logger };

    LARGE_INTEGER start;
    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&start);    

    while (!window.shouldClose()) {
        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);
        auto elapsed = float(now.QuadPart - start.QuadPart) / frequency.QuadPart;

        render.begin(elapsed);

        render.end();
        glfwPollEvents();
    }

    render.close();
    glfwTerminate();

    return 0;
}

int WINAPI WinMain(UNUSED HINSTANCE hInstance, UNUSED HINSTANCE hPrevInstance, UNUSED LPSTR CmdLine, UNUSED int nCmdShow) {
    return commonMain();
}

int main(UNUSED int argc, UNUSED const char **argv) {
    return commonMain();
}
