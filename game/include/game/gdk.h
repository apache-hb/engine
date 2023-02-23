#pragma once

#include <unordered_map>

#include "simcoe/core/win32.h"
#include "simcoe/core/util.h"

#include "XSystem.h"
#include "XGameRuntimeFeature.h"

namespace game::gdk {
    namespace util = simcoe::util;

    struct FeatureInfo {
        const char *name;
        bool enabled;
    };

    using FeatureMap = std::unordered_map<XGameRuntimeFeature, FeatureInfo>;

    void init();
    void deinit();
    bool enabled();

    FeatureMap& getFeatures();

    struct Runtime {
        Runtime();
        ~Runtime();

    private:
        std::unique_ptr<util::Entry> debug;
    };
}
