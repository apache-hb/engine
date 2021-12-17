#include "logging/log.h"
#include "system/system.h"
#include "render/render.h"
#include "util/strings.h"
#include "util/timer.h"
#include "input/inputx.h"

#include <thread>

using namespace engine;

using WindowCallbacks = system::Window::Callbacks;

struct MainWindow final : WindowCallbacks {
    using WindowCallbacks::WindowCallbacks;

    MainWindow() {
        factory = new render::Factory(DXGI_CREATE_FACTORY_DEBUG);
        context = new render::Context(factory);
    }

    virtual void onCreate(system::Window *ctx) override {
        render::Context::Create info = {
            .adapter = factory->adapter(0),
            .window = ctx,
            .frames = 2
        };

        context->createDevice(info);
        context->createAssets();

        draw = new std::jthread([this](auto stop) { 
            try {
                while (!stop.stop_requested()) { 
                    context->present();
                }
            } catch (const engine::Error &error) {
                log::global->fatal("{}", error.query());
            } catch (...) {
                log::global->fatal("unknown error");
            }
        });
    }

    virtual void onDestroy() override {
        delete draw;
        delete context;
    }

    virtual void onKeyPress(int key) override {
        auto state = input.poll();
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

private:
    std::jthread *draw;
    xinput::Device input{0};
    render::Factory *factory;
    render::Context *context;
};

int runEngine(HINSTANCE instance, int show) {
    render::debug::enable();

    MainWindow callbacks;

    system::Window::Create create = {
        .title = TEXT("Hello world!"),
        .rect = system::rectCoords(0, 0, 1920 * 2, 1080 * 2),
        .style = system::Window::Style::BORDERLESS
    };

    auto window = system::createWindow(instance, create, &callbacks);
    window.run(show);

    render::debug::disable();

    return 0;
}

int commonMain(HINSTANCE instance, int show) noexcept {
    system::init();

    if (auto err = log::ConsoleChannel::init(); err) {
        log::global->info("ConsoleChannel::init() = %d", err);
    }

    int result = 99;

    try { 
        result = runEngine(instance, show); 
    } catch (const engine::Error &error) {
        log::global->fatal("caught engine::Error\n{}", error.query());
    } catch (const std::exception &err) {
        log::global->fatal("caught std::exception\n{}", err.what());
    } catch (...) {
        log::global->fatal("caught unknown exception");
    }

    log::global->info("graceful exit");

    return result;
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR CmdLine, int nCmdShow) {
    return commonMain(hInstance, nCmdShow);
}

int main(int argc, const char **argv) {
    return commonMain(GetModuleHandle(nullptr), SW_SHOW);
}
