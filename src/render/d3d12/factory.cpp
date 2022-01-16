#include "render/objects/factory.h"

import RenderDebug;

using namespace engine::render;

constexpr auto kTearingFeatureFlag = DXGI_FEATURE_PRESENT_ALLOW_TEARING;
constexpr auto kTearingSwapChainFlag = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
constexpr auto kTearingPresentFlag = DXGI_PRESENT_ALLOW_TEARING;

Resource SwapChain3::getBuffer(UINT index) {
    ID3D12Resource* resource = nullptr;
    HRESULT hr = get()->GetBuffer(index, IID_PPV_ARGS(&resource));
    check(hr, "failed to get back buffer");
    return resource;
}

Adapter::Adapter(IDXGIAdapter1* adapter): Super(adapter) {
    check(adapter->GetDesc1(&desc), "failed to get adapter desc");
}

Device Adapter::newDevice(D3D_FEATURE_LEVEL level) {
    log::render->info("creating device with adapter {}", strings::narrow(getName()));

    ID3D12Device4* device = nullptr;
    HRESULT hr = D3D12CreateDevice(get(), level, IID_PPV_ARGS(&device));        

#if ENABLE_AGILITY
    if (hr == D3D12_ERROR_INVALID_REDIST) {
        throw Error(hr, "failed to create device, agility sdk failed to load");
    }
#endif
    check(hr, "failed to create device");

    return device;
}

std::wstring_view Adapter::getName() const {
    return desc.Description;
}

VideoMemoryInfo Adapter::getMemoryInfo() {
    // check if we have the interface needed
    auto it = as<IDXGIAdapter3>();
    if (!it.valid()) { return { }; }

    // try and get memory if we do
    DXGI_QUERY_VIDEO_MEMORY_INFO info;
    HRESULT hr = it->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &info);
    if (FAILED(hr)) { return { }; }

    // then return the info
    return { 
        .budget = units::Memory(info.Budget), 
        .used = units::Memory(info.CurrentUsage), 
        .available = units::Memory(info.AvailableForReservation), 
        .committed = units::Memory(info.CurrentReservation) 
    };
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

SwapChain3 Factory::newSwapChain(CommandQueue queue, HWND handle, const DXGI_SWAP_CHAIN_DESC1& desc) {
    Com<IDXGISwapChain1> swapChain;
    check(get()->CreateSwapChainForHwnd(queue.get(), handle, &desc, nullptr, nullptr, &swapChain), "failed to create swap chain");
    check(get()->MakeWindowAssociation(handle, DXGI_MWA_NO_ALT_ENTER), "failed to make window association");
    return SwapChain3(swapChain.as<IDXGISwapChain3>().get());
}
