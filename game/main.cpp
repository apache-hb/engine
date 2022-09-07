#include "engine/logging/logging.h"
#include "engine/platform/platform.h"
#include "engine/base/macros.h"

#include <windows.h>
#include <iostream>

using namespace engine;

int commonMain(UNUSED HINSTANCE instance, UNUSED int show) noexcept {
    platform::System machine;
    std::cout << "name: " << machine.name << std::endl;
    for (const auto &display : machine.displays) {
        std::cout << "display[" << display.name << "]: " << display.desc << std::endl;
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, UNUSED HINSTANCE hPrevInstance, UNUSED LPSTR CmdLine, int nCmdShow) {
    return commonMain(hInstance, nCmdShow);
}

int main(UNUSED int argc, UNUSED const char **argv) {
    return commonMain(GetModuleHandle(nullptr), SW_SHOW);
}
