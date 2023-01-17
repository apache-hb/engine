#include <windows.h>

#include "simcoe/core/logging.h"
#include "simcoe/core/window.h"

using namespace simcoe;

int commonMain() {
    logging::ConsoleSink console { "console" };
    logging::FileSink file { "file", "game.log" };

    logging::Category category { logging::eInfo, "general" };
    category.addSink(&console);
    category.addSink(&file);

    logging::fatal(category, "hello, {}!", "world");

    platform::Window::Size size { 800, 600 };
    platform::Window window { "Game", size };

    while (window.poll()) {
        // do stuff
    }

    return 0;
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    return commonMain();
}

int main(int, const char **) {
    return commonMain();
}
