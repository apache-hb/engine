#include "context.h"

namespace engine::render {
    Context::Context() {

    }

    Context::~Context() {

    }

    void Context::selectAdapter(size_t index) {
        adapterIndex = index;
        auto adapter = currentAdapter();

        render->info("selecting adapter {}: {}", index, adapter.name());

        ensure(D3D12CreateDevice(adapter.get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device)), "d3d12-create-device");
    }
}
