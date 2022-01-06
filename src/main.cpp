#include "logging/log.h"
#include "system/system.h"
#include "render/render.h"
#include "render/objects/factory.h"
#include "render/viewport/viewport.h"
#include "render/scene/scene.h"
#include "render/debug/debug.h"
#include "util/strings.h"
#include "util/timer.h"
#include "input/inputx.h"
#include "input/mnk.h"
#include "util/macros.h"

#include <thread>

#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_win32.h"
#include "imgui/backends/imgui_impl_dx12.h"

using namespace engine;

constexpr xinput::Deadzone kLStickDeadzone = { XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE };
constexpr xinput::Deadzone kRStickDeadzone = { XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE };
constexpr int16_t kLTriggerDeadzone = XINPUT_GAMEPAD_TRIGGER_THRESHOLD;
constexpr int16_t kRTriggerDeadzone = XINPUT_GAMEPAD_TRIGGER_THRESHOLD;

constexpr float kLookSensitivity = 1.f;
constexpr float kMoveSensitivity = 1.f;
constexpr float kUpDownSensitivity = 1.f;

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
            .getScene = [=](auto* ctx) -> render::Scene* { return new render::Scene3D(ctx, &camera); }
        });
        
        poll = new std::jthread([this](auto stop) {
            auto timer = util::createTimer();
            while (!stop.stop_requested()) {
                auto delta = timer.tick();
                auto data = input.poll();

                camera.move(data.lstick.x * delta * kMoveSensitivity, data.lstick.y * delta * kMoveSensitivity, (data.rtrigger - data.ltrigger) * delta * kUpDownSensitivity);
                camera.rotate(data.rstick.x * delta * kLookSensitivity, data.rstick.y * delta * kLookSensitivity);
            }
        });

        draw = new std::jthread([this](auto stop) { 
            try {
                context->create();

                while (!stop.stop_requested()) {
                    if (!context->present()) {
                        log::global->fatal("present failed");
                        break;
                    }
                }
            } catch (const engine::Error& error) {
                log::global->fatal("{}", error.what());
            }

            context->destroy();
        });
    }

    virtual void onDestroy() override {
        delete poll;
        delete draw;
        delete context;
    }

private:
    std::jthread *poll;
    std::jthread *draw;

    xinput::Device input{0, kLStickDeadzone, kRStickDeadzone, kLTriggerDeadzone, kRTriggerDeadzone};
    render::Camera camera = { { 0.f, 0.f, 0.f }, { 0.f, 0.f, 1.f } };
    render::Context *context;
};

void initImGui(size_t dpi) {
    ImGuiIO &io = ImGui::GetIO();
    io.Fonts->AddFontFromFileTTF("resources/DroidSans.ttf", float(dpi) / (96.f / 13.f));
    io.DisplayFramebufferScale = { dpi / 96.f, dpi / 96.f };
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
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
