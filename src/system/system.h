#pragma once

#include "util/win32.h"
#include "util/units.h"

namespace engine::system {
    struct Window {
        struct Callbacks {
            virtual void onCreate(Window *ctx) { }
            virtual void onDestroy() { }
            virtual void onKeyPress(int key) { }
            virtual void onKeyRelease(int key) { }
            virtual void repaint() { }
        };

        DWORD run(int show);

        using Size = std::tuple<LONG, LONG>;

        win32::Result<RECT> getClientRect() const;
        win32::Result<Size> getClientSize() const;

        HWND getHandle() const { return handle; }
        LPCTSTR getName() const { return name; }
        HINSTANCE getInstance() const { return instance; }
        Callbacks *getCallbacks() const { return callbacks; }

        Window(HINSTANCE instance, LPCTSTR name, HWND handle, Callbacks *callbacks)
            : instance(instance)
            , name(name)
            , handle(handle)
            , callbacks(callbacks) 
        { }
        
    private:
        HINSTANCE instance;
        LPCTSTR name;
        HWND handle;

        Callbacks *callbacks;
    };

    win32::Result<Window> createWindow(HINSTANCE instance, LPCTSTR name, Window::Size size, Window::Callbacks *callbacks);
    void destroyWindow(Window &window);

    struct Stats {
        std::string name;
    };

    win32::Result<Stats> getStats();
}
