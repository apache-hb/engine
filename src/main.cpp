#include "window.h"
#include "render/context.h"

struct MainWindow final : WindowHandle {
    using WindowHandle::WindowHandle;

    MainWindow(HINSTANCE instance, int show)
        : WindowHandle(instance, show, TEXT("hello world"), 800, 600)
    { }

    virtual void onCreate() override {
        if (HRESULT hr = instance.create(); FAILED(hr)) {
            fprintf(stderr, "window: instance.create = 0x%x\n", hr);
            return;
        }

        if (HRESULT hr = context.create(instance, 0); FAILED(hr)) {
            fprintf(stderr, "window: context.create = 0x%x\n", hr);
            return;
        }

        if (HRESULT hr = context.attach(this, 2); FAILED(hr)) {
            fprintf(stderr, "window: context.attach = 0x%x\n", hr);
            return;
        }
    }

    virtual void onDestroy() override {
        context.detach();
        context.destroy();
        instance.destroy();
    }

    virtual void onKeyPress(int key) override {
        
    }

    virtual void onKeyRelease(int key) override {
        
    }

    virtual void repaint() override {
        
    }

    render::Instance instance;
    render::Context context;
};

int commonMain(HINSTANCE instance, int show) {
    MainWindow window(instance, show);

    window.run();

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
