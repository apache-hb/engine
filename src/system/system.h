#pragma once

#include <vector>

#include "util/win32.h"
#include "util/units.h"

namespace engine::system {
    struct Window {
        using Size = std::tuple<LONG, LONG>;

        enum class Style {
            WINDOWED,
            BORDERLESS
        };

        struct Create {
            LPCTSTR title;
            Size size;
            Style style = Style::WINDOWED;
        };

        struct Callbacks {
            virtual void onCreate(Window *ctx) { }
            virtual void onDestroy() { }
            virtual void onClose() { }
            virtual void onKeyPress(int key) { }
            virtual void onKeyRelease(int key) { }
            virtual void repaint() { }
        };

        DWORD run(int show);

        win32::Result<RECT> getClientRect() const;
        win32::Result<Size> getClientSize() const;
        win32::Result<float> getClientAspectRatio() const;

        void popup(std::string_view title, std::string_view message);
        void center();

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

    win32::Result<Window> createWindow(HINSTANCE instance, const Window::Create &create, Window::Callbacks *callbacks);
    void destroyWindow(Window &window);

    struct Stats {
        std::string name;
    };

    win32::Result<Stats> getStats();

    std::vector<std::string> loadedModules();
    std::vector<std::string> detrimentalModules(const std::vector<std::string>& all);

    struct Display {
        using Self = DISPLAY_DEVICEA;
        Display(Self device) : device(device) { }

        std::string_view name() const { return device.DeviceName; }
        std::string_view desc() const { return device.DeviceString; }

    private:
        Self device;
    };

    std::vector<Display> displays();

    void init();
}
