#include "simcoe/simcoe.h"

#include "simcoe/core/system.h"
#include "simcoe/input/desktop.h"
#include "simcoe/locale/locale.h"
#include "simcoe/render/context.h"

#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_win32.h"
#include "imgui/backends/imgui_impl_dx12.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

using namespace simcoe;

struct Window : system::Window {
    using system::Window::Window;

    input::Desktop kbm { false };

    bool poll() {
        kbm.updateMouse(getHandle());
        return system::Window::poll();
    }

    LRESULT onEvent(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) override {
        kbm.updateKeys(msg, wparam, lparam);

        return ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam);
    }
};

int commonMain() {
    system::init();

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    Locale locale;
    Window window { "game", { 1280, 720 } };
    
    render::Context::Info info = {
        .resolution = { 600, 800 }
    };
    render::Context context { window, info };

    auto& heap = context.getHeap();
    auto handle = heap.alloc();
    ASSERT(handle != render::Heap::Index::eInvalid);

    ImGui_ImplWin32_Init(window.getHandle());
    ImGui_ImplDX12_Init(
        context.getDevice(), 
        int(context.getFrames()), 
        DXGI_FORMAT_R8G8B8A8_UNORM, 
        heap.getHeap(),
        heap.cpuHandle(handle),
        heap.gpuHandle(handle)
    );

    while (window.poll()) {
        context.begin();
        ImGui_ImplDX12_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        ImGui::ShowDemoWindow();

        if (ImGui::Begin("Input")) {
            auto& keys = window.kbm.getKeys();
            for (size_t i = 0; i < keys.size(); ++i) {
                ImGui::Text("%s: %zu", locale.get(input::Key(i)), keys[i]);
            }
        }
        
        ImGui::End();

        ImGui::Render();

        ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), context.getCommandList());
        context.end();
    }

    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();

    gLog.info("bye bye");

    return 0;
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    return commonMain();
}

int main(int, const char **) {
    return commonMain();
}
