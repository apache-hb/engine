#include "imgui_impl_dx12.h"
#include "simcoe/simcoe.h"

#include "simcoe/core/system.h"
#include "simcoe/render/context.h"

#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_win32.h"

using namespace simcoe;

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

struct Window : system::Window {
    using system::Window::Window;

    LRESULT onEvent(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) override {
        return ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam);
    }
};

int commonMain() {
    system::init();

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    auto &category = gLogs[eGeneral];

    Window window { "game", { 1280, 720 } };
    
    render::Context::Info info = {
        .resolution = { 600, 800 }
    };
    render::Context context { window, info };

    ImGui_ImplWin32_Init(window.getHandle());
    // ImGui_ImplDX12_Init(context.getDevice(), context.getFrames());

    while (window.poll()) {
        ImGui_ImplDX12_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        ImGui::ShowDemoWindow();

        ImGui::Render();
        context.present();
    }

    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();

    category.info("bye bye");

    return 0;
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    return commonMain();
}

int main(int, const char **) {
    return commonMain();
}
