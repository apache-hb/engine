#include <windows.h>

#include "simcoe/core/system.h"
#include "simcoe/core/logging.h"

using namespace simcoe;

int commonMain() {
    system::init();

    logging::ConsoleSink console { "console" };
    logging::FileSink file { "file", "game.log" };

    logging::Category category { logging::eInfo, "general" };
    category.addSink(&console);
    category.addSink(&file);

    logging::fatal(category, "hello, {}!", "world");

    panic({ "", "", 0 }, "oh no!");

    return 0;
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    return commonMain();
}

int main(int, const char **) {
    return commonMain();
}
