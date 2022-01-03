#include "logging/log.h"
#include "system/system.h"
#include "render/render.h"
#include "render/debug/debug.h"
#include "util/strings.h"
#include "util/timer.h"
#include "input/inputx.h"
#include "input/camera.h"
#include "input/mnk.h"
#include "util/macros.h"
#include "assets/loader.h"

#include <thread>

#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_win32.h"
#include "imgui/backends/imgui_impl_dx12.h"

using namespace engine;

using WindowCallbacks = system::Window::Callbacks;

struct MainWindow final : WindowCallbacks {
    using WindowCallbacks::WindowCallbacks;

    virtual void onCreate(system::Window *window) override {
        context = new render::Context({
            .factory = new render::Factory(),
            .adapter = 0,
            .window = window,
            .buffers = 2,
            .resolution = { 1920 / 2, 1080 / 2 },
            .getViewport = [](auto* ctx) { return new render::DisplayViewport(ctx); },
            .getScene = [](auto* ctx) -> render::Scene* { return new render::Scene3D(ctx); }
        });
        
        draw = new std::jthread([this](auto stop) { 
            context->create();

            while (!stop.stop_requested()) {
                if (!context->present()) {
                    log::global->fatal("present failed");
                    break;
                }
            }

            context->destroy();
        });
    }

    virtual void onDestroy() override {
        delete draw;
        delete context;
    }

private:

    std::jthread *draw;

    render::Context *context;
};

void initImGui(size_t dpi) {
    ImGuiIO &io = ImGui::GetIO();
    io.Fonts->AddFontFromFileTTF("resources/DroidSans.ttf", float(dpi) / (96.f / 13.f));
    io.DisplayFramebufferScale = { dpi / 96.f, dpi / 96.f };
}

int runEngine(HINSTANCE instance, int show) {
    render::debug::enable();

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

    window.run(show);

    ImGui_ImplWin32_Shutdown();

    render::debug::report();
    render::debug::disable();

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
