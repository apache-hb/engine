#include "input.h"
#include "rebind.h"
#include "window.h"

struct MainWindow final : WindowHandle 
{
    using WindowHandle::WindowHandle;

    virtual void onCreate() override
    {
        MessageBox(get(), TEXT("created"), TEXT("event"), 0);
    }

    virtual void onDestroy() override
    {
        MessageBox(get(), TEXT("destroyed"), TEXT("event"), 0);
    }
};

int WINAPI wWinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPTSTR CmdLine,
    int nCmdShow
)
{
    MainWindow window { 
        hInstance, 
        nCmdShow, 
        TEXT("window"), 
        800, 600 
    };

    window.run();    
}
