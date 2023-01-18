#include "simcoe/core/system.h"
#include "simcoe/core/logging.h"
#include "simcoe/render/context.h"

#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_win32.h"

using namespace simcoe;

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

struct ImGuiWindow : system::Window {
    ImGuiWindow(const char *pzTitle, const system::Size& size, system::WindowStyle style = system::WindowStyle::eWindow) : system::Window(pzTitle, size, style) {
        ImGui_ImplWin32_Init(getHandle());
    }

    ~ImGuiWindow() {
        ImGui_ImplWin32_Shutdown();
    }

    LRESULT onEvent(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) override {
        return ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam);
    }
};

int commonMain() {
    system::init();

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    logging::ConsoleSink console { "console" };
    logging::FileSink file { "file", "game.log" };

    logging::Category category { logging::eInfo, "general" };
    category.addSink(&console);
    category.addSink(&file);

    ImGuiWindow window { "game", { 1280, 720 } };
    
    render::Context::Info info = {
        .resolution = { 600, 800 }
    };
    render::Context context { window, info };

    while (window.poll()) {
        context.present();
    }

    return 0;
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    return commonMain();
}

int main(int, const char **) {
    return commonMain();
}
