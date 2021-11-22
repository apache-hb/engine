#pragma once

#include <windows.h>

struct WindowHandle 
{
    WindowHandle() = delete;
    WindowHandle(const WindowHandle&) = delete;
    WindowHandle(WindowHandle&&) = delete;

    WindowHandle(
        HINSTANCE instance,
        int show,
        LPTSTR title, 
        LONG width, 
        LONG height
    );
    virtual ~WindowHandle();

    void run();

    virtual void onCreate() { }
    virtual void onDestroy() { }

    HWND get() const { return handle; }
private:
    HINSTANCE instance;
    LPTSTR name;
    HWND handle;
};
