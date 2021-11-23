#pragma once

#define UNICODE 1

#include <windows.h>

struct WindowHandle 
{
    WindowHandle() = delete;
    WindowHandle(const WindowHandle&) = delete;
    WindowHandle(WindowHandle&&) = delete;

    WindowHandle(
        HINSTANCE instance,
        int show,
        LPCTSTR title, 
        LONG width, 
        LONG height
    );
    virtual ~WindowHandle();

    void run();

    virtual void onCreate() { }
    virtual void onDestroy() { }

    virtual void onKeyPress(int key) { }
    virtual void onKeyRelease(int key) { }

    virtual void repaint() { }

    HWND get() const { return handle; }
private:
    HINSTANCE instance;
    LPCTSTR name;
    HWND handle;
};
