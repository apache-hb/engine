#include "simcoe/simcoe.h"

#include "simcoe/core/system.h"

#include "simcoe/input/desktop.h"

#include "simcoe/locale/locale.h"
#include "simcoe/audio/audio.h"
#include "simcoe/render/context.h"

#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_win32.h"
#include "imgui/backends/imgui_impl_dx12.h"

#include "simcoe/core/win32.h"

#include <windows.h>
#include <winerror.h>

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
    XSystemAnalyticsInfo info;
    bool features[sizeof(kFeatures) / sizeof(Feature)] = {};
};

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

    HR_CHECK(XGameRuntimeInitialize());

    Features features = {};

    features.info = XSystemGetAnalyticsInfo();

    for (size_t i = 0; i < kFeatures.size(); ++i) {
        features.features[i] = XGameRuntimeIsFeatureAvailable(kFeatures[i].feature);
    }

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

    while (window.poll()) {
        context.begin();
        ImGui_ImplDX12_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        ImGui::ShowDemoWindow();

        if (ImGui::Begin("GDK")) {
            auto [major, minor, build, revision] = features.info.osVersion;
            ImGui::Text("OS Version: %u.%u.%u.%u", major, minor, build, revision);
            ImGui::Text("Platform: %s:%s", features.info.family, features.info.form);
            
            for (size_t i = 0; i < kFeatures.size(); ++i) {
                ImGui::Text("%s: %s", kFeatures[i].name, features.features[i] ? "enabled" : "disabled");
            }
        }
        ImGui::End();

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
