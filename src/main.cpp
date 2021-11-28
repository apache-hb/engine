#include "window.h"
#include "dx12.h"

extern "C" { 
    __declspec(dllexport) extern const UINT D3D12SDKVersion = 4;
    __declspec(dllexport) extern const auto* D3D12SDKPath = u8".\\D3D12\\"; 
}

struct MainWindow final : WindowHandle {
    using WindowHandle::WindowHandle;

    MainWindow(HINSTANCE instance, int show)
        : WindowHandle(instance, show, TEXT("hello world"), 800, 600)
    { }

    virtual void onCreate() override {
        factory.create(true);
        context.create(&factory, 0);
        context.attach(this, 2);
        context.load();
    }

    virtual void onDestroy() override {
        context.unload();
        context.detach();
        context.destroy();
        factory.destroy();
    }

    virtual void onKeyPress(int key) override {
        
    }

    virtual void onKeyRelease(int key) override {
        
    }

    virtual void repaint() override {
        context.present();
    }

    render::Factory factory;
    render::Context context;
};

int WINAPI wWinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPTSTR CmdLine,
    int nCmdShow
)
{
    MainWindow window(hInstance, nCmdShow);

    window.run();
}

int main(int argc, const char **argv) {
    MainWindow window(GetModuleHandle(nullptr), SW_SHOW);

    window.run();
}
