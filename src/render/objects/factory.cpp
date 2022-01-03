#include "factory.h"

#include "render/debug/debug.h"

using namespace engine::render;

constexpr auto kTearingFeatureFlag = DXGI_FEATURE_PRESENT_ALLOW_TEARING;
constexpr auto kTearingSwapChainFlag = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
constexpr auto kTearingPresentFlag = DXGI_PRESENT_ALLOW_TEARING;

Adapter::Adapter(IDXGIAdapter1* adapter): Super(adapter) {
    check(adapter->GetDesc1(&desc), "failed to get adapter desc");
}

Object<ID3D12Device> Adapter::createDevice(D3D_FEATURE_LEVEL level, std::wstring_view name, REFIID riid) {
    log::render->info("creating device with adapter {}", strings::narrow(getName()));

    ID3D12Device* device = nullptr;
    HRESULT hr = D3D12CreateDevice(get(), level, riid, reinterpret_cast<void**>(&device));        

#if ENABLE_AGILITY
    if (hr == D3D12_ERROR_INVALID_REDIST) {
        throw Error(hr, "failed to create device, agility sdk failed to load");
    }
#endif
    if (FAILED(hr)) { throw render::Error(hr, "failed to create device"); }

    return { device, name };
}

std::wstring_view Adapter::getName() const {
    return desc.Description;
}

VideoMemoryInfo Adapter::getMemoryInfo() {
    if (auto it = as<IDXGIAdapter3>(); it.valid()) {
        DXGI_QUERY_VIDEO_MEMORY_INFO info;
        check(it->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &info), "failed to query memory info");
        return { 
            .budget = units::Memory(info.Budget), 
            .used = units::Memory(info.CurrentUsage), 
            .available = units::Memory(info.AvailableForReservation), 
            .committed = units::Memory(info.CurrentReservation) 
        };
    } else {
        return { };
    }
}

Factory::Factory() {
    check(CreateDXGIFactory2(kFactoryFlags, IID_PPV_ARGS(addr())), "failed to create factory");
    
    IDXGIAdapter1* adapter;
    for (UINT i = 0; get()->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND; i++) {
        adapters.emplace_back(adapter);
    }

    if (auto factory5 = as<IDXGIFactory5>(); factory5) {
        check(factory5->CheckFeatureSupport(kTearingFeatureFlag, &tearing, sizeof(tearing)), "failed to query tearing support");
    }
}

std::span<Adapter> Factory::getAdapters() {
    return adapters;
}

Adapter& Factory::getAdapter(size_t index) {
    return adapters[index];
}

UINT Factory::presentFlags() const {
    return tearing ? kTearingPresentFlag : 0;
}

UINT Factory::flags() const {
    return tearing ? kTearingSwapChainFlag : 0;
}

Com<IDXGISwapChain1> Factory::createSwapChain(Object<ID3D12CommandQueue> queue, HWND handle, const DXGI_SWAP_CHAIN_DESC1& desc) {
    Com<IDXGISwapChain1> swapChain;
    check(get()->CreateSwapChainForHwnd(queue.get(), handle, &desc, nullptr, nullptr, &swapChain), "failed to create swap chain");
    check(get()->MakeWindowAssociation(handle, DXGI_MWA_NO_ALT_ENTER), "failed to make window association");
    return swapChain;
}
