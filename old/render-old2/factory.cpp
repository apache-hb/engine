#include "factory.h"
#include "debug/debug.h"

namespace engine::render {
    ///
    /// adapters
    ///

    Adapter::Adapter(IDXGIAdapter* adapter)
        : Super(adapter) 
    { }

    Object<ID3D12Device> Adapter::createDevice(D3D_FEATURE_LEVEL level, std::wstring_view name, REFIID riid) {
        Object<ID3D12Device> device;
        HRESULT hr = D3D12CreateDevice(get(), level, riid, reinterpret_cast<void**>(&device));        
    
#if ENABLE_AGILITY
        if (hr == D3D12_ERROR_INVALID_REDIST) {
            throw Error(hr, "failed to create device, agility sdk failed to load");
        }
#endif
        if (FAILED(hr)) { throw render::Error(hr, "failed to create device"); }

        device.rename(name);

        return device;
    }



    ///
    /// factorys
    ///

    Factory::Factory() { 
        check(CreateDXGIFactory2(DEFAULT_FACTORY_FLAGS, IID_PPV_ARGS(addr())), "failed to create factory");
    
        IDXGIAdapter* adapter;
        for (UINT i = 0; get()->EnumAdapters(i, &adapter) != DXGI_ERROR_NOT_FOUND; i++) {
            adapters.push_back(Adapter(adapter));
        }

        if (auto factory5 = as<IDXGIFactory5>(); factory5) {
            check(factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &tearing, sizeof(tearing)), "failed to query tearing support");
        }
    }

    std::span<Adapter> Factory::getAdapters() {
        return adapters;
    }

    Adapter& Factory::getAdapter(size_t index) {
        return adapters[index];
    }

    UINT Factory::presentFlags() const {
        return tearing ? DXGI_PRESENT_ALLOW_TEARING : 0;
    }

    UINT Factory::flags() const {
        return tearing ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;
    }

    Com<IDXGISwapChain1> Factory::createSwapChain(Object<ID3D12CommandQueue> queue, HWND handle, const DXGI_SWAP_CHAIN_DESC1& desc) {
        Com<IDXGISwapChain1> swapChain;
        check(get()->CreateSwapChainForHwnd(queue.get(), handle, &desc, nullptr, nullptr, &swapChain), "failed to create swap chain");
        check(get()->MakeWindowAssociation(handle, DXGI_MWA_NO_ALT_ENTER), "failed to make window association");
        return swapChain;
    }
}
