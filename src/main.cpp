#include "window.h"
#include "dx12.h"

struct MainWindow final : WindowHandle {
    using WindowHandle::WindowHandle;

    MainWindow(HINSTANCE instance, int show)
        : WindowHandle(instance, show, TEXT("hello world"), 800, 600)
    { }

    virtual void onCreate() override {
        
    }

    virtual void onDestroy() override {
    
    }

    virtual void onKeyPress(int key) override {
        
    }

    virtual void onKeyRelease(int key) override {
        
    }
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
