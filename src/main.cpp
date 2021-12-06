#include "logging/log.h"
#include "system/system.h"
#include "render/render.h"
#include "util/strings.h"

using namespace engine;

using WindowCallbacks = system::Window::Callbacks;

logging::Channel *channel = new logging::ConsoleChannel("engine", stdout);

struct MainWindow final : WindowCallbacks {
    using WindowCallbacks::WindowCallbacks;

    virtual void onCreate(system::Window *ctx) override {
        window = ctx;
        channel->info("on-create");

        auto factory = render::createFactory();
        context = render::createContext(factory.value(), ctx, 0, 8);
        context.createCore();
        context.createAssets();

        auto warns = system::detrimentalModules(system::loadedModules());
        if (warns.size() > 0) {
            channel->warn("the following loaded dlls may intefere with the engines functionality");
            for (auto warn : warns) {
                channel->warn("\t{}", warn);
            }
        }
    }

    virtual void onDestroy() override {
        context.destroyAssets();
        context.destroyCore();
        channel->info("on-destroy");
    }

    virtual void onKeyPress(int key) override {
        channel->info("on-key-press: {}", key);
    }

    virtual void onKeyRelease(int key) override {
        channel->info("on-key-release: {}", key);
    }

    virtual void repaint() override {
        context.present();
    }

private:
    system::Window *window;
    render::Context context;
};

int commonMain(HINSTANCE instance, int show) {
    channel->info("agility sdk {}", AGILITY ? "enabled" : "disabled");
    if (render::debug::enable() != S_OK) {
        return 99;
    }

    auto stats = system::getStats();
    if (!stats) { return stats.error(); }
    channel->info("{}", stats.value().name);

    MainWindow callbacks;

    auto window = system::createWindow(instance, TEXT("Hello world"), { 800, 600 }, &callbacks);
    if (!window) { return window.error(); }
    window.value().run(show);

    render::debug::report();
    render::debug::disable();

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
