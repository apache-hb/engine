#pragma once

#include "device.h"
#include "util/units.h"

#include <dxgi1_6.h>

namespace engine::render {
    struct VideoMemoryInfo {
        units::Memory budget; /// how much memory windows wants us to use
        units::Memory used; /// how much memory is current in used
        units::Memory available; /// how much memory we can reserve
        units::Memory committed; /// how much memory is available only to us
    };

    using SwapChain1 = Com<IDXGISwapChain1>;

    struct SwapChain3 : Com<IDXGISwapChain3> {
        Resource getBuffer(UINT index);
    };

    struct Adapter : Com<IDXGIAdapter1> {
        using Super = Com<IDXGIAdapter1>;
        using Super::Super;

        Adapter(IDXGIAdapter1* adapter);

        Device newDevice(D3D_FEATURE_LEVEL level);

        std::wstring_view getName() const;
        VideoMemoryInfo getMemoryInfo();

    private:
        DXGI_ADAPTER_DESC1 desc;
    };

    struct Factory : Com<IDXGIFactory2> {
        using Super = Com<IDXGIFactory2>;

        Factory();

        std::span<Adapter> getAdapters();
        Adapter& getAdapter(size_t index);
        UINT presentFlags() const;
        UINT flags() const;

        SwapChain3 newSwapChain(CommandQueue queue, HWND handle, const DXGI_SWAP_CHAIN_DESC1& desc);

    private:
        std::vector<Adapter> adapters;
        BOOL tearing = false;
    };
}
