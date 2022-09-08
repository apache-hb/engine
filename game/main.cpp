#include "engine/base/io.h"
#include "engine/logging/logging.h"
#include "engine/platform/platform.h"
#include "engine/base/macros.h"

using namespace engine;

int commonMain(UNUSED HINSTANCE instance, UNUSED int show) {
    Io *logFile = Io::open("game.log", Io::eWrite);
    logging::IoChannel logger {"general", logFile};
    platform::System machine {instance};

    logger.info("name: {}", machine.name);

    auto *window = machine.primaryDisplay().open("game", { 640, 480 });

    machine.run();

    delete window;

    return 0;
}

int WINAPI WinMain(UNUSED HINSTANCE hInstance, UNUSED HINSTANCE hPrevInstance, UNUSED LPSTR CmdLine, int nCmdShow) {
    return commonMain(hInstance, nCmdShow);
}

int main(UNUSED int argc, UNUSED const char **argv) {
    return commonMain(GetModuleHandle(nullptr), SW_SHOW);
}
