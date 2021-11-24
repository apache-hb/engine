#pragma once

#include <vector>

#include "utils.h"

#include <d3d12.h>
#include <d3d12sdklayers.h>
#include <dxgi1_6.h>

namespace d3d12 {
    struct Viewport : D3D12_VIEWPORT {
        using Super = D3D12_VIEWPORT;

        Viewport(FLOAT width, FLOAT height)
            : Super({ 0.f, 0.f, width, height, 0.f, 0.f })
        { 
            ASSERT(width >= 0.f);
            ASSERT(height >= 0.f);
        }
    };

    struct Factory {
        void create(bool debugging);
        void destroy();

        ID3D12Debug1 *debug;
        IDXGIFactory4 *factory;
        std::vector<IDXGIAdapter1*> adapters;
    };

    struct Render {
        Render(Factory *factory, size_t adapter);
    
    private:
        IDXGIAdapter1 *getAdapter() const;
        Factory *factory;
        size_t index;
        ID3D12Device1 *device;
    };
}
