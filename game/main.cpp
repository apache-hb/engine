#include <windows.h>

#include "simcoe/core/system.h"
#include "simcoe/core/logging.h"
#include "simcoe/render/context.h"

using namespace simcoe;

int commonMain() {
    system::init();

    logging::ConsoleSink console { "console" };
    logging::FileSink file { "file", "game.log" };

    logging::Category category { logging::eInfo, "general" };
    category.addSink(&console);
    category.addSink(&file);

    system::Window window { "game", { 1280, 720 } };
    
    render::Context::Info info = {
        .resolution = { 600, 800 }
    };
    render::Context context { window, info };

    while (window.poll()) {
        context.present();
    }

    return 0;
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    return commonMain();
}

int main(int, const char **) {
    return commonMain();
}
