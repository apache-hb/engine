#include "engine/rhi/rhi.h"
#include "objects/common.h"

using namespace engine;
using namespace rhi;

constexpr GUID kShaderModel6 = { /* 76f5573e-f13a-40f5-b297-81ce9e18933f */
    0x76f5573e,
    0xf13a,
    0x40f5,
    { 0xb2, 0x97, 0x81, 0xce, 0x9e, 0x18, 0x93, 0x3f }
};

rhi::Device rhi::getDevice() {
    DX_CHECK(D3D12EnableExperimentalFeatures(1, &kShaderModel6, nullptr, nullptr));

    DX_CHECK(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&gDebug)));

    UINT factoryFlags = 0;

    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&gDxDebug)))) {
        gDxDebug->EnableDebugLayer();
        factoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
    }

    DX_CHECK(CreateDXGIFactory2(factoryFlags, IID_PPV_ARGS(&gFactory)));

    IDXGIAdapter1 *adapter;
    for (UINT i = 0; SUCCEEDED(gFactory->EnumAdapters1(i, &adapter)); i++) {
        DXGI_ADAPTER_DESC1 desc;
        adapter->GetDesc1(&desc);
        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            continue;

        break;
    }

    ID3D12Device *device;
    DX_CHECK(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device)));

    return rhi::Device(device);
}
