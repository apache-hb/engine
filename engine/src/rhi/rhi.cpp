#include "engine/rhi/rhi.h"
#include "dx/d3d12.h"
#include "objects/common.h"

using namespace simcoe;
using namespace simcoe::rhi;

rhi::Device rhi::getDevice() {
    auto &channel = logging::get(logging::eRender);

    DX_CHECK(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&gDebug)));

    UINT factoryFlags = 0;

    if (HRESULT hr = D3D12GetDebugInterface(IID_PPV_ARGS(&gDxDebug)); SUCCEEDED(hr)) {
        gDxDebug->EnableDebugLayer();
        factoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
    } else {
        channel.warn("faield to get d3d12 debug interface: {}", simcoe::hrErrorString(hr));
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

    D3D12_FEATURE_DATA_SHADER_MODEL shaderModel = { D3D_SHADER_MODEL_6_0 };
    if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &shaderModel, sizeof(D3D12_FEATURE_DATA_SHADER_MODEL))) || shaderModel.HighestShaderModel < D3D_SHADER_MODEL_6_0) {
        channel.warn("device does not support shader model 6.0");
    }

    return device;
}
