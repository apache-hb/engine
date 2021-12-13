#include "logging/log.h"
#include "system/system.h"
#include "render/render.h"
#include "util/strings.h"
#include "util/timer.h"

using namespace engine;

using WindowCallbacks = system::Window::Callbacks;

logging::Channel *channel = new logging::ConsoleChannel("engine", stdout);

struct MainWindow final : WindowCallbacks {
    using WindowCallbacks::WindowCallbacks;

    virtual void onCreate(system::Window *ctx) override {
        window = ctx;

        auto factory = render::createFactory();
        context = render::createContext(factory.value(), ctx, 0, 2);
        context.createCore();
        context.createAssets();
    
        timer = util::createTimer();
    }

    virtual void onDestroy() override {
        context.destroyAssets();
        context.destroyCore();
    }

    virtual void onKeyPress(int key) override {
        context.loseDevice();
    }

    virtual void onKeyRelease(int key) override {
    
    }

    virtual void onClose() override {
        channel->info("close prompt");
    }

    virtual void repaint() override {
        total += timer.tick();
        context.constBufferData.offset.x = cosf(total) / 2;
        context.constBufferData.offset.y = sinf(total) / 2;

        if (HRESULT hr = context.present(); FAILED(hr)) {
            if (!context.retry(1)) {
                channel->fatal("failed to recreate context");
                ExitProcess(99);
            }
        }
    }

private:
    float total = 0.f;
    util::Timer timer;
    system::Window *window;
    render::Context context;
};

int commonMain(HINSTANCE instance, int show) {
    system::init();

    if (auto err = logging::ConsoleChannel::init(); err) {
        channel->info("ConsoleChannel::init() = %d", err);
    }

    channel->info("agility sdk {}", AGILITY ? "enabled" : "disabled");
    if (render::debug::enable() != S_OK) {
        return 99;
    }

    auto stats = system::getStats();
    if (!stats) { return stats.error(); }
    channel->info("{}", stats.value().name);

    for (const auto &display : system::displays()) {
        channel->info("display {}: {}", display.name(), display.desc());
    }

    MainWindow callbacks;

    system::Window::Create create = {
        .title = TEXT("Hello world"),
        .size = { 800, 600 },
        .style = system::Window::Style::BORDERLESS
    };

    auto window = system::createWindow(instance, create, &callbacks);
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
