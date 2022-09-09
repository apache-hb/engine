#include "engine/base/macros.h"
#include "engine/base/io.h"
#include "engine/logging/logging.h"
#include "engine/platform/platform.h"
#include "engine/render/render.h"

#include <thread>

using namespace engine;

int commonMain(UNUSED HINSTANCE instance, UNUSED int show) {
    Io *logFile = Io::open("game.log", Io::eWrite);
    logging::IoChannel fileLogger {"general", logFile, logging::eFatal};
    logging::ConsoleChannel consoleLogger { "general" , logging::eDebug};

    logging::Channel *channels[] { &fileLogger, &consoleLogger };
    logging::MultiChannel logger { "general", channels };

    platform::System machine {instance};

    logger.info("name: {}", machine.name);

    auto window = machine.primaryDisplay().open("game", { 640, 480 });

    auto thread = std::jthread([&, window](auto stop) {
        while (!stop.stop_requested()) {
            auto event = window->getEvent();
            logger.debug("event {{ {}, {}, {} }}", event.msg, event.lparam, event.wparam);
        }
    });

    thread.detach();

    render::Context render { window };

    machine.run();

    thread.request_stop();

    return 0;
}

int WINAPI WinMain(UNUSED HINSTANCE hInstance, UNUSED HINSTANCE hPrevInstance, UNUSED LPSTR CmdLine, int nCmdShow) {
    return commonMain(hInstance, nCmdShow);
}

int main(UNUSED int argc, UNUSED const char **argv) {
    return commonMain(GetModuleHandle(nullptr), SW_SHOW);
}
