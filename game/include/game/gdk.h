#pragma once

#include <unordered_map>

#include "simcoe/core/win32.h"

#include "XSystem.h"
#include "XGameRuntimeFeature.h"

namespace game::gdk {
    struct FeatureInfo {
        const char *name;
        bool enabled;
    };

    using FeatureMap = std::unordered_map<XGameRuntimeFeature, FeatureInfo>;

    void init();
    void deinit();
    bool enabled();

    const XSystemAnalyticsInfo& getAnalyticsInfo();
    const char *getConsoleId();

    FeatureMap& getFeatures();

    struct Runtime {
        Runtime() { init(); }
        ~Runtime() { deinit(); }
    };
}
