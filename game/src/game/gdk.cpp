#include "game/gdk.h"
#include "game/registry.h"

#include "simcoe/core/system.h"

#include "simcoe/simcoe.h"

#include "XGameRuntime.h"
#include "XGameRuntimeFeature.h"

#include "imgui/imgui.h"

#include <array>

using namespace game;
using namespace simcoe;

namespace {
    bool gEnabled = false;
    gdk::FeatureMap gFeatures;

    XSystemAnalyticsInfo gInfo = {};
    std::array<char, 256> gConsoleId;
}

#define CHECK_FEATURE(key) \
    do { \
        gFeatures[XGameRuntimeFeature::key] = { .name = #key, .enabled = XGameRuntimeIsFeatureAvailable(XGameRuntimeFeature::key) }; \
    } while (false)

void gdk::init() {
    if (HRESULT hr = XGameRuntimeInitialize(); FAILED(hr)) {
        gLog.warn("XGameRuntimeInitialize() = {}", system::hrString(hr));
        return;
    }

    gInfo = XSystemGetAnalyticsInfo();

    size_t size = gConsoleId.size();
    HR_CHECK(XSystemGetConsoleId(gConsoleId.size(), gConsoleId.data(), &size));
    gConsoleId[size] = '\0';

    CHECK_FEATURE(XAccessibility);
    CHECK_FEATURE(XAppCapture);
    CHECK_FEATURE(XAsync);
    CHECK_FEATURE(XAsyncProvider);
    CHECK_FEATURE(XDisplay);
    CHECK_FEATURE(XGame);
    CHECK_FEATURE(XGameInvite);
    CHECK_FEATURE(XGameSave);
    CHECK_FEATURE(XGameUI);
    CHECK_FEATURE(XLauncher);
    CHECK_FEATURE(XNetworking);
    CHECK_FEATURE(XPackage);
    CHECK_FEATURE(XPersistentLocalStorage);
    CHECK_FEATURE(XSpeechSynthesizer);
    CHECK_FEATURE(XStore);
    CHECK_FEATURE(XSystem);
    CHECK_FEATURE(XTaskQueue);
    CHECK_FEATURE(XThread);
    CHECK_FEATURE(XUser);
    CHECK_FEATURE(XError);
    CHECK_FEATURE(XGameEvent);
    CHECK_FEATURE(XGameStreaming);

    gEnabled = true;
}

void gdk::deinit() {
    XGameRuntimeUninitialize();
}

bool gdk::enabled() {
    return gEnabled;
}

gdk::FeatureMap& gdk::getFeatures() {
    return gFeatures;
}

gdk::Runtime::Runtime() { 
    init(); 
    
    debug = game::debug.newEntry({ "GDK", gdk::enabled() }, [] {
        ImGui::Text("Family: %s", gInfo.family);
        ImGui::Text("Model: %s", gInfo.form);

        auto [major, minor, build, revision] = gInfo.osVersion;
        ImGui::Text("OS: %u.%u.%u.%u", major, minor, build, revision);
        ImGui::Text("ID: %s", gConsoleId.data());

        if (ImGui::BeginTable("features", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY)) {
            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableSetupColumn("Feature", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn("Available", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableHeadersRow();
            for (const auto& [feature, detail] : gdk::getFeatures()) {
                const auto& [name, available] = detail;

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("%s", name);
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%s", available ? "enabled" : "disabled");
            }
            ImGui::EndTable();
        }
    });
}

gdk::Runtime::~Runtime() { 
    deinit(); 
}
