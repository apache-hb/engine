#include "engine/base/io.h"
#include "engine/logging/logging.h"
#include "engine/platform/platform.h"
#include "engine/base/macros.h"

#include <windows.h>
#include <iostream>

using namespace engine;

int commonMain(UNUSED HINSTANCE instance, UNUSED int show) noexcept {
    Io *logFile = Io::open("game.log", Io::eWrite);
    logging::IoChannel logger {"general", logFile};
    platform::System machine;

    logger.info("name: {}", machine.name);
    for (const auto &display : machine.displays) {
        logger.info("display[{}]: {}", display.name, display.desc);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, UNUSED HINSTANCE hPrevInstance, UNUSED LPSTR CmdLine, int nCmdShow) {
    return commonMain(hInstance, nCmdShow);
}

int main(UNUSED int argc, UNUSED const char **argv) {
    return commonMain(GetModuleHandle(nullptr), SW_SHOW);
}
