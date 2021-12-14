#include "util/win32.h"
#include <stdio.h>
#include <source_location>

import engine.util.error;

#if 0
#include "logging/logs.h"
#include "system/system.h"
#include "render/render.h"
#include "util/strings.h"
#include "util/timer.h"

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

    }

    virtual void onKeyRelease(int key) override {
    
    }

    virtual bool shouldClose() override {
        return true;
    }

    virtual void repaint() override {

    }

private:
    system::Window *window;
};
#endif

int commonMain(HINSTANCE instance, int show) {
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
