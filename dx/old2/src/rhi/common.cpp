#include "engine/rhi/api.h"

using namespace engine;

rhi::Adapter *rhi::Instance::getBestAdapter() {
    std::span adapters = getAdapters();

    // get the first discrete adapter
    for (rhi::Adapter *adapter : adapters) {
        if (adapter->getType() == rhi::Adapter::eDiscrete) {
            return adapter;
        }
    }

    // otherwise return the first
    return adapters[0];
}
