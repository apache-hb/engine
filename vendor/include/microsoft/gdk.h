#pragma once

#include <unordered_map>
#include <string_view>

#include "simcoe/core/win32.h"

#include "XSystem.h"
#include "XGameRuntimeFeature.h"

namespace vendor::microsoft::gdk {
    struct FeatureInfo {
        std::string_view name;
        bool enabled;
    };

    // update this if more gdk feature sets are added
    using FeatureArray = std::array<FeatureInfo, 22>;

    void init();
    void deinit();
    bool enabled();

    XSystemAnalyticsInfo& getAnalyticsInfo();
    FeatureArray& getFeatures();
    std::string_view getConsoleId();

    struct Runtime {
        Runtime();
        ~Runtime();
    };
}
