#include "engine/base/macros.h"
#include "engine/base/io.h"
#include "engine/logging/logging.h"
#include "engine/render/render.h"

#include "engine/base/win32.h"

#include <GLFW/glfw3.h>

#include <gl/gl.h>
#include <thread>

using namespace engine;

struct Action {
    template<typename F>
    Action(F&& func) : self(func) { }
    ~Action() { 
        self.request_stop();
    }

private:
    std::jthread self;
};

int commonMain() {
    // we want to be dpi aware
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    glfwInit();

    std::unique_ptr<Io> logFile { Io::open("game.log", Io::eWrite) };
    logging::IoChannel fileLogger {"general", logFile.get(), logging::eFatal};
    logging::ConsoleChannel consoleLogger { "general", logging::eDebug};

    logging::Channel *channels[] { &fileLogger, &consoleLogger };
    logging::MultiChannel logger { "general", channels };

    // we wont be using opengl so dont init it
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);

    // make a fullscreen borderless window on the primary monitor
    const auto *mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    auto *window = glfwCreateWindow(mode->width, mode->height, "game", nullptr, nullptr);

    glfwSetKeyCallback(window, +[](GLFWwindow *, int, int, int, int) {

    });

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }

    glfwTerminate();

    return 0;
}

int WINAPI WinMain(UNUSED HINSTANCE hInstance, UNUSED HINSTANCE hPrevInstance, UNUSED LPSTR CmdLine, UNUSED int nCmdShow) {
    return commonMain();
}

int main(UNUSED int argc, UNUSED const char **argv) {
    return commonMain();
}
