#pragma once

#include <vector>

#include "util/win32.h"
#include "util/units.h"

namespace engine::system {
    RECT rectCoords(LONG x, LONG y, LONG width, LONG height);

    struct Window {
        using Size = std::tuple<LONG, LONG>;

        enum class Style {
            WINDOWED,
            BORDERLESS
        };

        struct Create {
            LPCTSTR title;
            RECT rect;
            Style style = Style::WINDOWED;
        };

        struct Callbacks {
            virtual void onCreate(Window *ctx) { }
            virtual void onDestroy() { }
            virtual bool onClose() { return true; }
            virtual void onKeyPress(int key) { }
            virtual void onKeyRelease(int key) { }
            virtual void repaint() { }
        };

        DWORD run(int show);

        RECT getClientRect() const;
        Size getClientSize() const;
        float getClientAspectRatio() const;

        void popup(std::string_view title, std::string_view message);

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

    Window createWindow(HINSTANCE instance, const Window::Create &create, Window::Callbacks *callbacks);
    void destroyWindow(Window &window);

    struct Stats {
        std::string name;
    };

    Stats getStats();

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
