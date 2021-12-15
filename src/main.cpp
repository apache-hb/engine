#include "logging/log.h"
#include "system/system.h"
#include "render/render.h"
#include "util/strings.h"
#include "util/timer.h"
#include "input/inputx.h"

using namespace engine;

using WindowCallbacks = system::Window::Callbacks;

struct MainWindow final : WindowCallbacks {
    using WindowCallbacks::WindowCallbacks;

    virtual void onCreate(system::Window *ctx) override {
        window = ctx;
    }

    virtual void onDestroy() override {
        
    }

    virtual void onKeyPress(int key) override {
        auto state = device.poll();
        auto gamepad = state.Gamepad;
        log::global->info("packet {} ({:x} {} {} ({}x{}) ({}x{}))", 
            state.dwPacketNumber, gamepad.wButtons, 
            gamepad.bLeftTrigger, gamepad.bRightTrigger,
            gamepad.sThumbLX, gamepad.sThumbLY,
            gamepad.sThumbRX, gamepad.sThumbRY
        );
    }

    virtual void onKeyRelease(int key) override {
    
    }

    virtual void repaint() override {
        
    }

private:
    system::Window *window;
    xinput::Device device{0};
};

int commonMain(HINSTANCE instance, int show) {
    system::init();

    if (auto err = log::ConsoleChannel::init(); err) {
        log::global->info("ConsoleChannel::init() = %d", err);
    }

    MainWindow callbacks;

    system::Window::Create create = {
        .title = TEXT("Hello world!"),
        .rect = system::rectCoords(0, 0, 800, 600),
        .style = system::Window::Style::WINDOWED
    };

    auto window = system::createWindow(instance, create, &callbacks);
    if (!window) { return window.error(); }
    window.value().run(show);

    return 0;
}

int WINAPI wWinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPTSTR CmdLine,
    int nCmdShow
)
{
    return commonMain(hInstance, nCmdShow);
}

int main(int argc, const char **argv) {
    return commonMain(GetModuleHandle(nullptr), SW_SHOW);
}
