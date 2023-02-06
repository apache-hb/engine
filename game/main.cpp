#include "simcoe/core/system.h"
#include "simcoe/core/win32.h"

#include "simcoe/input/desktop.h"
#include "simcoe/input/gamepad.h"

#include "simcoe/locale/locale.h"
#include "simcoe/audio/audio.h"
#include "simcoe/render/context.h"
#include "simcoe/render/graph.h"

#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_win32.h"
#include "imgui/backends/imgui_impl_dx12.h"

#include "simcoe/simcoe.h"

#include "GameInput.h"
#include "XGameRuntime.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

using namespace simcoe;

struct Feature {
    XGameRuntimeFeature feature;
    const char *name;
};

constexpr auto kFeatures = std::to_array<Feature>({
    { XGameRuntimeFeature::XAccessibility, "XAccessibility" },
    { XGameRuntimeFeature::XAppCapture, "XAppCapture" },
    { XGameRuntimeFeature::XAsync, "XAsync" },
    { XGameRuntimeFeature::XAsyncProvider, "XAsyncProvider" },
    { XGameRuntimeFeature::XDisplay, "XDisplay" },
    { XGameRuntimeFeature::XGame, "XGame" },
    { XGameRuntimeFeature::XGameInvite, "XGameInvite" },
    { XGameRuntimeFeature::XGameSave, "XGameSave" },
    { XGameRuntimeFeature::XGameUI, "XGameUI" },
    { XGameRuntimeFeature::XLauncher, "XLauncher" },
    { XGameRuntimeFeature::XNetworking, "XNetworking" },
    { XGameRuntimeFeature::XPackage, "XPackage" },
    { XGameRuntimeFeature::XPersistentLocalStorage, "XPersistentLocalStorage" },
    { XGameRuntimeFeature::XSpeechSynthesizer, "XSpeechSynthesizer" },
    { XGameRuntimeFeature::XStore, "XStore" },
    { XGameRuntimeFeature::XSystem, "XSystem" },
    { XGameRuntimeFeature::XTaskQueue, "XTaskQueue" },
    { XGameRuntimeFeature::XThread, "XThread" },
    { XGameRuntimeFeature::XUser, "XUser" },
    { XGameRuntimeFeature::XError, "XError" },
    { XGameRuntimeFeature::XGameEvent, "XGameEvent" },
    { XGameRuntimeFeature::XGameStreaming, "XGameStreaming" },
});

struct Features {
    Features() {
        size_t size = 0;
        HR_CHECK(XSystemGetConsoleId(sizeof(id), id, &size));

        info = XSystemGetAnalyticsInfo();

        for (size_t i = 0; i < kFeatures.size(); ++i) {
            features[i] = XGameRuntimeIsFeatureAvailable(kFeatures[i].feature);
        }
    }

    XSystemAnalyticsInfo info;
    bool features[kFeatures.size()] = {};
    char id[128] = {};
};

struct Info {
    Features features;
    Locale locale;

    input::Gamepad gamepad;
    input::Keyboard keyboard;
    input::Mouse mouse;

    input::Manager manager;
};

struct Window : system::Window {
    Window(Info& info)
        : system::Window("game", { 1280, 720 })
        , info(info) 
    { }

    bool poll() {
        info.mouse.update(getHandle());
        return system::Window::poll();
    }

    LRESULT onEvent(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) override {
        info.keyboard.update(msg, wparam, lparam);

        return ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam);
    }

    Info& info;
};

struct ImGuiPass final : render::Pass {
    ImGuiPass(const GraphObject& object, Info& info)
        : Pass(object)
        , info(info)
    { }
    
    void start() override {
        auto& context = getContext();
        auto& heap = context.getHeap();
        fontHandle = heap.alloc();

        ImGui_ImplDX12_Init(
            context.getDevice(),
            int(context.getFrames()),
            DXGI_FORMAT_R8G8B8A8_UNORM,
            heap.getHeap(),
            heap.cpuHandle(fontHandle),
            heap.gpuHandle(fontHandle)
        );
    }

    void stop() override {
        ImGui_ImplDX12_Shutdown();
        getContext().getHeap().release(fontHandle);
    }

    void execute() override {
        ImGui_ImplDX12_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        ImGui::ShowDemoWindow();
        
        if (ImGui::Begin("GDK")) {
            const auto& features = info.features;
            ImGui::Text("family: %s", features.info.family);
            ImGui::SameLine();
            auto [major, minor, build, revision] = features.info.osVersion;
            ImGui::Text("os: %u.%u.%u.%u", major, minor, build, revision);
            ImGui::Text("id: %s", features.id);

            if (ImGui::BeginTable("features", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY)) {
                ImGui::TableSetupScrollFreeze(0, 1);
                ImGui::TableSetupColumn("Feature", ImGuiTableColumnFlags_WidthFixed);
                ImGui::TableSetupColumn("Available", ImGuiTableColumnFlags_WidthFixed);
                ImGui::TableHeadersRow();
                for (size_t i = 0; i < kFeatures.size(); ++i) {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("%s", kFeatures[i].name);
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%s", features.features[i] ? "enabled" : "disabled");
                }
                ImGui::EndTable();
            }
        }
        ImGui::End();

        constexpr auto kTableFlags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY | ImGuiTableFlags_NoHostExtendX;
        const float kTextWidth = ImGui::CalcTextSize("A").x;

        if (ImGui::Begin("Input")) {
            const auto& state = info.manager.getState();
            
            ImGui::Text("Current: %s", state.device == input::Device::eGamepad ? "Gamepad" : "Mouse & Keyboard");

            if (ImGui::BeginTable("keys", 2, kTableFlags, ImVec2(kTextWidth * 40, 0.0f))) {
                ImGui::TableSetupScrollFreeze(0, 1);
                ImGui::TableSetupColumn("Key", ImGuiTableColumnFlags_WidthFixed);
                ImGui::TableSetupColumn("Index", ImGuiTableColumnFlags_WidthFixed);
                ImGui::TableHeadersRow();
                for (size_t i = 0; i < state.key.size(); i++) {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("%s", info.locale.get(input::Key(i)));
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%zu", state.key[i]);
                }
                ImGui::EndTable();
            }

            ImGui::SameLine();

            if (ImGui::BeginTable("axis", 2, kTableFlags, ImVec2(kTextWidth * 45, 0.0f))) {
                ImGui::TableSetupScrollFreeze(0, 1);
                ImGui::TableSetupColumn("Axis", ImGuiTableColumnFlags_WidthFixed);
                ImGui::TableSetupColumn("Offset", ImGuiTableColumnFlags_WidthFixed);
                ImGui::TableHeadersRow();
                for (size_t i = 0; i < state.axis.size(); i++) {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("%s", info.locale.get(input::Axis(i)));
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%f", state.axis[i]);
                }
                ImGui::EndTable();
            }
        }
        ImGui::End();

        ImGui::Render();
        ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), getContext().getCommandList());
    }

private:
    Info& info;
    render::Heap::Index fontHandle = render::Heap::Index::eInvalid;
};

struct Scene final : render::Graph {
    Scene(render::Context& context, Info& info) : render::Graph { context } {
        pImGuiPass = addPass<ImGuiPass>("imgui", info);
    }

    void execute() {
        Graph::execute(pImGuiPass);
    }

private:
    ImGuiPass *pImGuiPass;
};

int commonMain() {
    system::init();

    HR_CHECK(XGameRuntimeInitialize());

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    Info detail = {
        .gamepad = input::Gamepad { 0 },
        .keyboard = input::Keyboard { },
        .mouse = input::Mouse { false, true },
    };

    detail.manager.add(&detail.gamepad);
    detail.manager.add(&detail.keyboard);
    detail.manager.add(&detail.mouse);

    Window window { detail };

    render::Context::Info info = {
        .resolution = { 600, 800 }
    };
    render::Context context { window, info };
    Scene scene { context, detail };

    ImGui_ImplWin32_Init(window.getHandle());

#if 0
    audio::Context sound;

    XSpeechSynthesizerHandle speech;
    XSpeechSynthesizerStreamHandle stream;

    HR_CHECK(XSpeechSynthesizerCreate(&speech));
    HR_CHECK(XSpeechSynthesizerSetDefaultVoice(speech));

    HR_CHECK(XSpeechSynthesizerCreateStreamFromText(speech, "Hello world", &stream));

    size_t bufferSize = 0;
    HR_CHECK(XSpeechSynthesizerGetStreamDataSize(stream, &bufferSize));

    std::vector<std::byte> buffer{bufferSize};

    HR_CHECK(XSpeechSynthesizerGetStreamData(stream, bufferSize, buffer.data(), &bufferSize));

    buffer.resize(bufferSize);

    sound.play((void*)buffer.data(), bufferSize);
#endif

    scene.start();

    while (window.poll()) {
        detail.manager.poll();
        scene.execute();
    }

    scene.stop();

    ImGui_ImplWin32_Shutdown();

    XGameRuntimeUninitialize();

    gLog.info("bye bye");

    return 0;
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    return commonMain();
}

int main(int, const char **) {
    return commonMain();
}
