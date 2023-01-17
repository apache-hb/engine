#pragma once

#include <vector>
#include <array>
#include <queue>

#include "util/macros.h"
#include "util/win32.h"
#include "util/units.h"
#include "atomic-queue/queue.h"

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

        struct Event {
            UINT type;
            WPARAM wparam;
            LPARAM lparam;
        };

        struct Callbacks {
            virtual void onCreate(UNUSED Window *ctx) { }
            virtual void onDestroy() { }
            virtual bool onClose() { return true; }

            bool hasEvent() const { return !events.is_empty(); }
            Event pollEvent() { return events.pop(); }
            void addEvent(Event event) { events.push(event); }

        private:
            ubn::queue<Event> events;
        };

        DWORD run(int show);

        RECT getClientRect() const;
        Size getClientSize() const;
        float getClientAspectRatio() const;
        size_t getDpi() const;

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
        friend Window createWindow(HINSTANCE instance, const Window::Create &create, Window::Callbacks *callbacks);

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

    struct Module {
        Module(std::wstring_view path);

        bool found() const;

        template<typename T>
        T* get(std::string_view name) const {
            return reinterpret_cast<T*>(getAddr(name));
        }

    private:
        void* getAddr(std::string_view name) const;
        HMODULE handle;
    };

    void init();
}
