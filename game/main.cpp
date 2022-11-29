#include "engine/base/macros.h"
#include "engine/base/io/io.h"
#include "engine/base/logging.h"
#include "engine/base/win32.h"
#include "engine/base/util.h"

#include "engine/input/input.h"

#include "engine/locale/locale.h"

#include "engine/math/consts.h"
#include "engine/render/scene.h"

#include "engine/ui/ui.h"

#include "engine/audio/audio.h"

#include "imgui.h"

using namespace simcoe;
using namespace simcoe::math;
using namespace simcoe::input;
using namespace simcoe::audio;

int commonMain() {
    win32::init();  
    logging::init();

    // make a fullscreen borderless window on the primary monitor
    int width = GetSystemMetrics(SM_CXSCREEN);
    int height = GetSystemMetrics(SM_CYSCREEN);

    UniquePtr<Window> window = ui::init({
        .title = "game",
        .size = { width, height },
        .imgui = "game.ini"
    });

    input::Keyboard keyboard { false };
    input::Gamepad gamepad0 { 0 };
    input::Gamepad gamepad1 { 1 };

    input::Manager manager { { &keyboard, &gamepad0, &gamepad1 }, { } };

    audio::Audio audio;

    render::ContextInfo info {
        .window = window.get(),
        .frames = 2,
        .resolution = { 640, 480 },
    };
    render::Context context { info };

    render::WorldGraphInfo worldInfo {
        .ctx = context,
        .update = [&] { 
            ImGui::ShowDemoWindow();
        }
    };

    render::WorldGraph world { worldInfo };

    world.init();

    while (window->poll(&keyboard)) {
        manager.poll();
        world.execute();
    }

    world.deinit();

    return 0;
}

template<typename F>
struct Defer {
    Defer(F func) : func(func) { }
    ~Defer() { func(); }

    F func;
};

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    Defer _ { [] { logging::get(logging::eGeneral).info("clean exit from wWinMain"); } };
    return commonMain();
}

int main(int, const char **) {
    Defer _ { [] { logging::get(logging::eGeneral).info("clean exit from main"); } };
    return commonMain();
}
