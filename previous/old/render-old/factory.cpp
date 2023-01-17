#include "factory.h"
#include "debug/debug.h"

#include "util/strings.h"

namespace engine::render {
    
    Output::Output(IDXGIOutput *output): self(output) {
        check(self->GetDesc(&desc), "failed to get output description");
    }

    std::string Output::name() const {
        return strings::narrow(desc.DeviceName);
    }

    RECT Output::coords() const {
        return desc.DesktopCoordinates;
    }

    Adapter::Adapter(IDXGIAdapter1 *adapter): self(adapter) { 
        if (!adapter) { return; }

        check(self->GetDesc1(&desc), "failed to get adapter description");
    
        IDXGIOutput *output;
        for (UINT i = 0; adapter->EnumOutputs(i, &output) != DXGI_ERROR_NOT_FOUND; ++i) {
            displays.push_back(Output(output));
        }
    }

    std::string Adapter::name() const {
        return strings::narrow(desc.Description);
    }

    units::Memory Adapter::videoMemory() const {
        return units::Memory(desc.DedicatedVideoMemory);
    }

    units::Memory Adapter::systemMemory() const {
        return units::Memory(desc.DedicatedSystemMemory);
    }

    units::Memory Adapter::sharedMemory() const {
        return units::Memory(desc.SharedSystemMemory);
    }

    std::span<Output> Adapter::outputs() {
        return std::span{displays};
    }

    bool Adapter::software() const {
        return desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE;
    }

    Factory::Factory() {
        UINT flags = DEFAULT_FACTORY_FLAGS;

        check(CreateDXGIFactory2(flags, IID_PPV_ARGS(&self)), "failed to create DXGI factory");

        IDXGIAdapter1 *adapter;
        for (UINT i = 0; self->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND; i++) {
            devices.push_back(Adapter(adapter));
        }

        if (auto factory5 = self.as<IDXGIFactory5>(); factory5.valid()) {
            check(factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &tearing, sizeof(tearing)), "failed to check for tearing support");
        }
    }

    Com<IDXGISwapChain1> Factory::createSwapChain(Com<ID3D12CommandQueue> queue, HWND window, const DXGI_SWAP_CHAIN_DESC1 &desc) {
        Com<IDXGISwapChain1> swapChain;
        check(self->CreateSwapChainForHwnd(queue.get(), window, &desc, nullptr, nullptr, &swapChain), "failed to create swap chain");
        check(self->MakeWindowAssociation(window, DXGI_MWA_NO_ALT_ENTER), "failed to make window association");
        return swapChain;
    }

    std::span<Adapter> Factory::adapters() {
        return std::span{devices};
    }

    Adapter Factory::adapter(size_t index) { 
        return devices[index];
    }

    UINT Factory::presentFlags() const {
        return tearing ? DXGI_PRESENT_ALLOW_TEARING : 0;
    }
}
