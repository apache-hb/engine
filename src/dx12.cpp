#include "dx12.h"

namespace d3d12 {
    void Factory::create(bool debugging) {
        UINT flags = 0;
        if (debugging) {
            ASSERT(SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug))));
            debug->EnableDebugLayer();
            flags = DXGI_CREATE_FACTORY_DEBUG;
        }

        ASSERT(SUCCEEDED(CreateDXGIFactory2(flags, IID_PPV_ARGS(&factory))));

        IDXGIAdapter1 *adapter;
        for (UINT i = 0; factory->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND; i++) {
            adapters.push_back(adapter);
        }
    }

    void Factory::destroy() {
        for (auto *adapter : adapters) {
            ASSERT(adapter->Release() == 0);
        }
        ASSERT(factory->Release() == 0);
        ASSERT(debug->Release() == 0);
    }

    Render::Render(Factory *factory, size_t adapter) {
        
    }
}
