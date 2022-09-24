#include "engine/rhi/rhi.h"

#include "objects/device.h"

using namespace engine;

rhi::Device *rhi::getDevice() {
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

    D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = { D3D_ROOT_SIGNATURE_VERSION_1_1 };
    if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData)))) {
        featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
    }

    return new DxDevice(device, featureData.HighestVersion);
}
