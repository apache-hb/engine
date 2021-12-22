#pragma once

#include "util.h"
#include "device.h"

namespace engine::render {
    struct Output {
        Output(IDXGIOutput *output = nullptr);

        std::string name() const;
        RECT coords() const;

    private:
        Com<IDXGIOutput> self;
        DXGI_OUTPUT_DESC desc;
    };

    struct Adapter {
        Adapter(IDXGIAdapter1 *adapter = nullptr);

        template<typename T>
        Device<T> newDevice(D3D_FEATURE_LEVEL level, std::wstring_view name) {
            T* device = nullptr;
            check(D3D12CreateDevice(self.get(), level, IID_PPV_ARGS(&device)), "failed to create device");
            Device<T> result(device);
            result.rename(name);
            return result;
        }

        std::string name() const;
        units::Memory videoMemory() const;
        units::Memory systemMemory() const;
        units::Memory sharedMemory() const;
        std::span<Output> outputs();
        bool software() const;

        auto get() { return self.get(); }

    private:
        Com<IDXGIAdapter1> self;
        DXGI_ADAPTER_DESC1 desc;
        std::vector<Output> displays;
    };

    struct Factory {
        Factory();
        
        std::span<Adapter> adapters();
        Adapter adapter(size_t index);
        UINT presentFlags() const;

        Com<IDXGISwapChain1> createSwapChain(Com<ID3D12CommandQueue> queue, HWND window, const DXGI_SWAP_CHAIN_DESC1 &desc);

    private:
        Com<IDXGIFactory4> self;
        std::vector<Adapter> devices;
        BOOL tearing = false;
    };
}
