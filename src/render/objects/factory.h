#pragma once

#include "device.h"

#include <dxgi1_6.h>

namespace engine::render {
    struct Adapter : Com<IDXGIAdapter1> {
        using Super = Com<IDXGIAdapter1>;
        using Super::Super;

        Adapter(IDXGIAdapter1* adapter);

        template<IsDevice T>
        Device<T> newDevice(D3D_FEATURE_LEVEL level, std::wstring_view name) {
            return Device<T>(createDevice(level, name, __uuidof(T)));
        }

        std::wstring_view getName() const;

    private:
        DXGI_ADAPTER_DESC1 desc;

        Object<ID3D12Device> createDevice(D3D_FEATURE_LEVEL level, std::wstring_view name, REFIID riid);
    };

    struct Factory : Com<IDXGIFactory2> {
        using Super = Com<IDXGIFactory2>;

        Factory(const Factory&) = delete;

        Factory();

        std::span<Adapter> getAdapters();
        Adapter& getAdapter(size_t index);
        UINT presentFlags() const;
        UINT flags() const;

        Com<IDXGISwapChain1> createSwapChain(Object<ID3D12CommandQueue> queue, HWND handle, const DXGI_SWAP_CHAIN_DESC1& desc);

    private:
        std::vector<Adapter> adapters;
        BOOL tearing = false;
    };
}
