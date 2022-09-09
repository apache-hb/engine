#pragma once

#include "engine/base/win32.h"
#include "engine/math/math.h"

#include <string>
#include <vector>

#include "atomic-queue/queue.h"

namespace engine::platform {
    using Size = math::Resolution<long>;
    using Position = math::Vec2<long>;

    constexpr auto kCenter = Position::of(LONG_MAX);

    struct Event {
        UINT msg;
        WPARAM wparam;
        LPARAM lparam;
    };

    struct Window {
        friend struct Display;

        Window() : events(512) { }
        Window(const Window&) = delete;
        Window(Window&&) = delete;

        void addEvent(Event event);
        Event getEvent();
    private:
        void create(HINSTANCE instance, const char *title, Size size, Position position);

        ubn::queue<Event> events;
        HWND window;
    };

    struct Display {
        friend struct System;

        Display() = delete;

        Window *open(const std::string &title, Size windowSize, Position windowPos = kCenter);

        Size size;
        std::string name;

    private:
        Display(HINSTANCE instance, Size size, Position position, std::string name)
            : size(size)
            , name(name)
            , position(position)
            , instance(instance)
        { }

        Position position;
        HINSTANCE instance;
    };

    struct System {
        System(HINSTANCE instance);

        System() = delete;
        System(const System&) = delete;
        System(System&&) = delete;

        void run();

        Display &primaryDisplay();

        std::string name;
        std::vector<Display> displays;

    private:
        size_t primary;
        HINSTANCE instance;
    };
}
