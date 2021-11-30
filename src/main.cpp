#include "window.h"
#include "render/context.h"
#include "logging/log.h"

using namespace engine;

struct MainWindow final : WindowHandle {
    using WindowHandle::WindowHandle;

    MainWindow(HINSTANCE instance, int show)
        : WindowHandle(instance, show, TEXT("hello world"), 800, 600)
        , channel("main-window", stdout)
    { }

    virtual void onCreate() override {
        for (const auto &adapter : context.adapters()) {
            channel.info("(name: {}, video-memory: {}, shared-memory: {}, system-memory: {}, luid: {})",
                adapter.name(), 
                adapter.video().string(),
                adapter.shared().string(),
                adapter.system().string(),
                adapter.luid().name()
            );
        }
    }

    virtual void onDestroy() override {
        
    }

    virtual void onKeyPress(int key) override {
        
    }

    virtual void onKeyRelease(int key) override {
        
    }

    virtual void repaint() override {
        
    }

    logging::ConsoleChannel channel;
    render::Context context;
};

int commonMain(HINSTANCE instance, int show) {
    logging::ConsoleChannel channel("main", stdout);
    
    try {
        logging::ConsoleChannel::init();

        MainWindow window(instance, show);

        window.run();
    } catch (const Error &error) {
        channel.fatal("{}", error.msg());
    }

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
