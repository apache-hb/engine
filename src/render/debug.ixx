module;

#include <d3dcompiler.h>
#include <dxgidebug.h>
#include <dxgi1_6.h>

#include "util.h"

export module RenderDebug;

#define ALWAYS_SHADER_FLAGS D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_WARNINGS_ARE_ERRORS

#if D3D12_DEBUG
#   define DEFAULT_FACTORY_FLAGS DXGI_CREATE_FACTORY_DEBUG
#   define DEFAULT_SHADER_FLAGS ALWAYS_SHADER_FLAGS | D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION
#else
#   define DEFAULT_FACTORY_FLAGS 0
#   define DEFAULT_SHADER_FLAGS ALWAYS_SHADER_FLAGS
#endif

engine::render::Com<ID3D12Debug> d3dDebug;
engine::render::Com<IDXGIDebug> dxgiDebug;

export namespace engine::render {
    constexpr auto kFactoryFlags = DEFAULT_FACTORY_FLAGS;
    constexpr auto kShaderFlags = DEFAULT_SHADER_FLAGS;
}

export namespace engine::render::debug {
    void enable() {
        if (HRESULT hr = DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug)); FAILED(hr)) {
            log::render->warn("failed to get dxgi debug interface: {}", toString(hr));
        }

        if (HRESULT hr = D3D12GetDebugInterface(IID_PPV_ARGS(&d3dDebug)); FAILED(hr)) {
            log::render->warn("failed to get d3d12 debug interface: {}", toString(hr));
        }

        if (!d3dDebug) { return; }

        d3dDebug->EnableDebugLayer();
        log::render->info("debug layer enabled");
    }

    void disable() {
        d3dDebug.tryDrop("d3d-debug");
    }

    void report() {
        if (!dxgiDebug) { return; }
        
        dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
    }
}
