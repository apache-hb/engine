#include "engine/render/window.h"
#include "GLFW/glfw3.h"

using namespace engine;

Window::Window(int width, int height, const char *title) { 
    // we wont be using opengl so dont init it
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);

    window = glfwCreateWindow(width, height, title, nullptr, nullptr);
}

math::Vec2<int> Window::size() {
    int left, top, right, bottom;
    glfwGetWindowFrameSize(window, &left, &top, &right, &bottom);

    return { (right - left), (bottom - top) };
}

HWND Window::handle() {
    return glfwGetWin32Window(window);
}

bool Window::shouldClose() {
    return glfwWindowShouldClose(window);
}
