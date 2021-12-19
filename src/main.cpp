#include "logging/log.h"
#include "system/system.h"
#include "render/render.h"
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

constexpr xinput::Deadzone ldeadzone = { XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE };
constexpr xinput::Deadzone rdeadzone = { XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE };
constexpr int16_t ltrigger = XINPUT_GAMEPAD_TRIGGER_THRESHOLD;
constexpr int16_t rtrigger = XINPUT_GAMEPAD_TRIGGER_THRESHOLD;

constexpr float moveSpeed = 5.f;
constexpr float ascentSpeed = 5.f;
constexpr float rotateSpeed = 3.f;

struct InputManager {
    void handleEvent(system::Window::Event event) {
        switch (event.type) {
        case WM_KEYDOWN: mnk.pushPress(event.wparam); break;
        case WM_KEYUP: mnk.pushRelease(event.wparam); break;
        default: break;
        }
    }

    void update() {
        auto in = poll();
        float delta = timer.tick();

        auto pitch = in.rstick.x * rotateSpeed * delta;
        auto yaw = in.rstick.y * rotateSpeed * delta;

        auto moveX = in.lstick.x * moveSpeed * delta;
        auto moveY = in.lstick.y * moveSpeed * delta;
        auto moveZ = (in.rtrigger - in.ltrigger) * ascentSpeed * delta;

        camera.rotate(pitch, yaw);
        camera.move(moveX, moveY, moveZ);
    }

    input::Input poll() {
        if (choice == XINPUT) {
            return device.poll();
        } else {
            return mnk.poll();
        }
    }

    enum Choice { XINPUT, MNK } choice = XINPUT;
    xinput::Device device{0, ldeadzone, rdeadzone, ltrigger, rtrigger};
    input::MnK mnk{0.1f};

    input::Camera camera{{0.f, 0.f, 0.f}, {1.f, 0.f, 0.f}};
    util::Timer timer = util::createTimer();
};

struct MainWindow final : WindowCallbacks {
    using WindowCallbacks::WindowCallbacks;

    MainWindow() {
        factory = new render::Factory(DXGI_CREATE_FACTORY_DEBUG);
        context = new render::Context(factory);
    }

    virtual void onCreate(system::Window *ctx) override {
        const auto frames = 2;
        render::Context::Create info = {
            .adapter = factory->adapter(0),
            .window = ctx,
            .frames = frames
        };

        context->createDevice(info);
        context->createAssets();

        input = new std::jthread([this](auto stop) {
            while (!stop.stop_requested()) {
                if (hasEvent()) { 
                    manager.handleEvent(pollEvent());
                }

                manager.update();
            }
        });

        draw = new std::jthread([this](auto stop) { 
            try {
                while (!stop.stop_requested()) {
                    context->tick(manager.camera);
                    context->present();
                }
            } catch (const engine::Error &error) {
                log::global->fatal("{}", error.query());
            } catch (...) {
                log::global->fatal("unknown error");
            }
        });
    }

    virtual void onDestroy() override {
        delete input;
        delete draw;
        delete context;
    }

private:
    std::jthread *draw;
    std::jthread *input;

    render::Factory *factory;
    render::Context *context;

    InputManager manager;
};

void initImGui(size_t dpi) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    ImGuiIO &io = ImGui::GetIO();
    io.Fonts->AddFontFromFileTTF("resources/DroidSans.ttf", float(dpi) / (96.f / 13.f));
    io.DisplayFramebufferScale = { dpi / 96.f, dpi / 96.f };
}

int runEngine(HINSTANCE instance, int show) {
    render::debug::enable();

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

    window.run(show);

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
