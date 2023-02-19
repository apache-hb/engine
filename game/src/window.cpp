#include "game/window.h"

#include "imgui/imgui.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

using namespace game;

Window::Window(input::Mouse& mouse, input::Keyboard& keyboard)
    : system::Window("game", { 1920, 1080 })
    , mouse(mouse)
    , keyboard(keyboard)
{ }

bool Window::poll() {
    mouse.update(getHandle());
    return system::Window::poll();
}

LRESULT Window::onEvent(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    keyboard.update(msg, wparam, lparam);

    return ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam);
}
