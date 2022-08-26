#include "logging/log.h"
#include "logging/debug.h"

#include "system/system.h"
#include "system/gamesdk.h"

#include "util/strings.h"
#include "util/timer.h"
#include "util/macros.h"

#include "input/inputx.h"
#include "input/mnk.h"

#include "assets/loader.h"

#include <thread>
#include <atomic>

#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_win32.h"

using namespace engine;

using WindowCallbacks = system::Window::Callbacks;

struct MainWindow final : WindowCallbacks {
    using WindowCallbacks::WindowCallbacks;

    virtual void onCreate(UNUSED system::Window *window) override {
        
    }

    virtual void onDestroy() override {
        delete draw;
    }

private:
    std::jthread* draw;
};

void initImGui(size_t dpi) {
    ImGuiIO &io = ImGui::GetIO();
    io.Fonts->AddFontFromFileTTF("resources/DroidSans.ttf", float(dpi) / (96.f / 13.f));
    io.DisplayFramebufferScale = { dpi / 96.f, dpi / 96.f };
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
}

int runEngine(HINSTANCE instance, int show) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    MainWindow callbacks;

    auto width = GetSystemMetrics(SM_CXSCREEN);
    auto height = GetSystemMetrics(SM_CYSCREEN);

    system::Window::Create create = {
        .title = TEXT("Hello world!"),
        .rect = system::rectCoords(0, 0, width, height),
        .style = system::Window::Style::BORDERLESS
    };

    auto window = system::createWindow(instance, create, &callbacks);

    initImGui(window.getDpi());

    ImGui_ImplWin32_Init(window.getHandle());

    discord::create();
    window.run(show);
    discord::destroy();

    ImGui_ImplWin32_Shutdown();

    ImGui::DestroyContext();

    return 0;
}

int commonMain(HINSTANCE instance, int show) noexcept {
    system::init();

    if (auto err = log::ConsoleChannel::init(); err) {
        log::global->info("ConsoleChannel::init() = %d", err);
    }

    int result = 99;

    try { 
        result = runEngine(instance, show); 
    } catch (const engine::Error &error) {
        log::global->fatal("caught engine::Error\n{}", error.query());
        log::global->fatal("{}:{}:{}:{}", error.file(), error.function(), error.line(), error.column());
    } catch (const std::exception &err) {
        log::global->fatal("caught std::exception\n{}", err.what());
    } catch (...) {
        log::global->fatal("caught unknown exception");
    }

    log::global->info("graceful exit");

    return result;
}

int WINAPI wWinMain(HINSTANCE hInstance, UNUSED HINSTANCE hPrevInstance, UNUSED LPTSTR CmdLine, int nCmdShow) {
    return commonMain(hInstance, nCmdShow);
}

int main(UNUSED int argc, UNUSED const char **argv) {
    return commonMain(GetModuleHandle(nullptr), SW_SHOW);
}
