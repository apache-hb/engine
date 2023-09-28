#include "game/events.h"

// imgui makes me forward declare this
#include "imgui/imgui.h"
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);


// impl

using namespace game;

LRESULT GameEvents::onEvent(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    keyboard.update(msg, wparam, lparam);

    return ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam);
}
