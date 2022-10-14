#include "engine/ui/ui.h"

#include "imgui.h"

using namespace simcoe;

Window *ui::init(const ui::Create& info) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    auto& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_DockingEnable;
    io.IniFilename = info.imgui;

    ImGui::StyleColorsDark();

    auto [width, height] = info.size;
    auto* window = new Window { width, height, info.title };

    auto dpi = window->dpi();
    io.Fonts->AddFontFromFileTTF("resources/DroidSans.ttf", float(dpi) / (96.f / 13.f));
    io.DisplayFramebufferScale = { dpi / 96.f, dpi / 96.f };

    return window;
}
