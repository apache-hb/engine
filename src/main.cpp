#include "rebind.h"
#include "window.h"

#include <atomic>
#include <thread>
#include <string>
#include <sstream>
#include <stop_token>

struct MainWindow final : WindowHandle 
{
    using WindowHandle::WindowHandle;

    MainWindow(HINSTANCE instance, int show)
        : WindowHandle(instance, show, TEXT("window"), 800, 600)
    { }

    virtual void onCreate() override
    {
        renderThread = new std::jthread(renderTask, this);
    }

    virtual void onDestroy() override
    {
        delete renderThread;
    }

    virtual void onKeyPress(int key) override
    {
        
    }

    virtual void onKeyRelease(int key) override
    {
        
    }

    virtual void repaint() override
    {
        
    }

    static void renderTask(std::stop_token stop, MainWindow* window)
    {
        while (!stop.stop_requested())
        {
            MessageBeep(MB_OK);
        }
    }

    std::jthread *renderThread;
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
