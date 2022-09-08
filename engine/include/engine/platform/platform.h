#pragma once

#include "engine/math/math.h"

#include <string>
#include <vector>

#include <windows.h>

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

        Window() = default;
        Window(const Window&) = delete;
        Window(Window&&) = delete;

        virtual ~Window() = default;

    private:
        void create(HINSTANCE instance, const char *title, Size size, Position position);

        HWND window;
    };

    template<typename T>
    concept IsWindow = std::is_base_of<Window, T>::value;

    struct Display {
        friend struct System;

        Display() = delete;

        template<IsWindow T = Window>
        T *open(const std::string &title, Size windowSize, Position windowPos = kCenter) {
            if (windowPos == kCenter) {
                windowPos = Position::from((size.width - windowSize.width) / 2, (size.height - windowSize.height) / 2);
            }
            
            auto *it = new T();
            it->create(instance, title.c_str(), windowSize, windowPos + position);
            return it;
        }

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
