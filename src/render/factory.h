#pragma once

#include "objects/device.h"

namespace engine::render {
    struct Adapter : Com<IDXGIAdapter> {
        Adapter();
        ~Adapter();

    private:
        DXGI_ADAPTER_DESC1 desc;
    };

    struct Factory : Com<IDXGIFactory> {
        Factory();
        ~Factory();

        std::span<Adapter> getAdapters();
        Adapter getAdapter(size_t index);
        UINT presentFlags() const;

        Com<IDXGISwapChain1> createSwapChain();

    private:
        std::vector<Adapter> adapters;
        BOOL tearing = false;
    };
}
