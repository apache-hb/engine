#include "render/debug.h"
#include "render/util.h"

#include <dxgidebug.h>

namespace engine::render::debug {
    Com<ID3D12Debug> d3dDebug;
    Com<IDXGIDebug> dxgiDebug;

    void enable() {
        if (HRESULT hr = DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug)); FAILED(hr)) {
            log::render->warn("failed to get dxgi debug interface: {}", toString(hr));
        }

        if (HRESULT hr = D3D12GetDebugInterface(IID_PPV_ARGS(&d3dDebug)); FAILED(hr)) {
            log::render->warn("failed to get d3d12 debug interface: {}", toString(hr));
        }

        if (d3dDebug) { d3dDebug->EnableDebugLayer(); }

        log::render->info("debug layer enabled");
    }

    void disable() {
        d3dDebug.tryDrop("d3d-debug");
    }

    void report() {
        if (dxgiDebug) {
            dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
        }
    }
}
