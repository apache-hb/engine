#include "game/gdk.h"

#include "simcoe/core/system.h"

#include "XGameRuntime.h"
#include "XGameRuntimeFeature.h"

#include <array>

using namespace game;

namespace {
    gdk::FeatureMap gEnabled;

    XSystemAnalyticsInfo gInfo;
    std::array<char, 256> gConsoleId;
}

#define CHECK_FEATURE(key) \
    do { \
        gEnabled[XGameRuntimeFeature::key] = { .name = #key, .enabled = XGameRuntimeIsFeatureAvailable(XGameRuntimeFeature::key) }; \
    } while (false)

void gdk::init() {
    HR_CHECK(XGameRuntimeInitialize());

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
}

void gdk::deinit() {
    XGameRuntimeUninitialize();
}

const XSystemAnalyticsInfo& gdk::getAnalyticsInfo() {
    return gInfo;
}

const char *gdk::getConsoleId() {
    return gConsoleId.data();
}

gdk::FeatureMap& gdk::getFeatures() {
    return gEnabled;
}
