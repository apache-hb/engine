#include "engine/rhi/rhi.h"

#include "objects/common.h"

using namespace simcoe;
using namespace simcoe::rhi;

#define STRING_CASE(x) case x: return #x

namespace {
    std::string dxCategoryStr(D3D12_MESSAGE_CATEGORY category) {
        switch (category) {
        STRING_CASE(D3D12_MESSAGE_CATEGORY_APPLICATION_DEFINED);
        STRING_CASE(D3D12_MESSAGE_CATEGORY_MISCELLANEOUS);
        STRING_CASE(D3D12_MESSAGE_CATEGORY_INITIALIZATION);
        STRING_CASE(D3D12_MESSAGE_CATEGORY_CLEANUP);
        STRING_CASE(D3D12_MESSAGE_CATEGORY_COMPILATION);
        STRING_CASE(D3D12_MESSAGE_CATEGORY_STATE_CREATION);
        STRING_CASE(D3D12_MESSAGE_CATEGORY_STATE_SETTING);
        STRING_CASE(D3D12_MESSAGE_CATEGORY_STATE_GETTING);
        STRING_CASE(D3D12_MESSAGE_CATEGORY_RESOURCE_MANIPULATION);
        STRING_CASE(D3D12_MESSAGE_CATEGORY_EXECUTION);
        STRING_CASE(D3D12_MESSAGE_CATEGORY_SHADER);
        default: return "<error>";
        }
    }

    std::string dxSeverityStr(D3D12_MESSAGE_SEVERITY severity) {
        switch (severity) {
        STRING_CASE(D3D12_MESSAGE_SEVERITY_CORRUPTION);
        STRING_CASE(D3D12_MESSAGE_SEVERITY_ERROR);
        STRING_CASE(D3D12_MESSAGE_SEVERITY_WARNING);
        STRING_CASE(D3D12_MESSAGE_SEVERITY_INFO);
        STRING_CASE(D3D12_MESSAGE_SEVERITY_MESSAGE);
        default: return "<error>";
        }
    }

    std::string dxIdStr(D3D12_MESSAGE_ID id) {
        return std::format("{}", (int)id);
    }

    void rhiDebugCallback(D3D12_MESSAGE_CATEGORY category, D3D12_MESSAGE_SEVERITY severity, D3D12_MESSAGE_ID id, LPCSTR desc, void *) {
        auto& channel = logging::v2::get(logging::eRender);

        switch (severity) {
        default:
        case D3D12_MESSAGE_SEVERITY_CORRUPTION:
        case D3D12_MESSAGE_SEVERITY_ERROR:
            channel.fatal("d3d12(category={}, severity={}, id={}, desc=\"{}\")", dxCategoryStr(category), dxSeverityStr(severity), dxIdStr(id), desc);
            break;

        case D3D12_MESSAGE_SEVERITY_WARNING:
            channel.warn("d3d12(category={}, severity={}, id={}, desc=\"{}\")", dxCategoryStr(category), dxSeverityStr(severity), dxIdStr(id), desc);
            break;

        case D3D12_MESSAGE_SEVERITY_INFO:
        case D3D12_MESSAGE_SEVERITY_MESSAGE:
            channel.info("d3d12(category={}, severity={}, id={}, desc=\"{}\")", dxCategoryStr(category), dxSeverityStr(severity), dxIdStr(id), desc);
            break;
        }
    }
}

rhi::Device rhi::getDevice() {
    auto &channel = logging::v2::get(logging::eRender);

    HR_CHECK(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&gDebug)));

    UINT factoryFlags = 0;

    if (HRESULT hr = D3D12GetDebugInterface(IID_PPV_ARGS(&gDxDebug)); SUCCEEDED(hr)) {
        gDxDebug->EnableDebugLayer();
        factoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
    } else {
        channel.warn("faield to get d3d12 debug interface: {}", simcoe::win32::hrErrorString(hr));
    }

    HR_CHECK(CreateDXGIFactory2(factoryFlags, IID_PPV_ARGS(&gFactory)));

    IDXGIAdapter1 *adapter;
    for (UINT i = 0; SUCCEEDED(gFactory->EnumAdapters1(i, &adapter)); i++) {
        DXGI_ADAPTER_DESC1 desc;
        adapter->GetDesc1(&desc);
        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            continue;

        break;
    }

    ID3D12Device *device = nullptr;
    HR_CHECK(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device)));

    D3D12_FEATURE_DATA_SHADER_MODEL shaderModel = { D3D_SHADER_MODEL_6_0 };
    if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &shaderModel, sizeof(D3D12_FEATURE_DATA_SHADER_MODEL))) || shaderModel.HighestShaderModel < D3D_SHADER_MODEL_6_0) {
        channel.warn("device does not support shader model 6.0");
    }

    ID3D12InfoQueue1 *pInfo = nullptr;
    if (HRESULT err = device->QueryInterface(IID_PPV_ARGS(&pInfo)); SUCCEEDED(err)) {
        DWORD cookie = UINT_MAX;
        HR_CHECK(pInfo->RegisterMessageCallback(rhiDebugCallback, D3D12_MESSAGE_CALLBACK_FLAG_NONE, nullptr, &cookie));

        channel.info("registered d3d12 debug callback (cookie={})", cookie);
        pInfo->Release();
    } else {
        channel.warn("failed to get info queue: {}", simcoe::win32::hrErrorString(err));
    }

    return device;
}
