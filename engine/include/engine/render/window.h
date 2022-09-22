#pragma once

#include "engine/math/math.h"

#define GLFW_EXPOSE_NATIVE_WIN32 1
#include <glfw/glfw3.h>
#include <GLFW/glfw3native.h>

namespace engine {
    struct Window {
        Window(int width, int height, const char *title);

        math::Vec2<int> size();
        HWND handle();

        bool shouldClose();

    private:
        GLFWwindow *window;
    };
}
