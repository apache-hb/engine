#include "engine/rhi/api.h"

#include "engine/base/panic.h"
#include "engine/base/util.h"

#include <comdef.h>

#include <dxgidebug.h>
#include <dxgi1_6.h>

#include "dx/d3dx12.h"
#include "dx/d3d12sdklayers.h"

#include <string>

using namespace engine;

namespace {
    void drop(IUnknown *pUnknown, std::string_view name) {
        ULONG refs = pUnknown->Release();
        ASSERTF(refs == 0, "{} live references for {}", refs, name);
    }

    std::string hrErrorString(HRESULT hr) {
        _com_error err(hr);
        return err.ErrorMessage();
    }
}

#define DX_CHECK(expr) do { HRESULT hr = (expr); ASSERTF(SUCCEEDED(hr), #expr " => {}", hrErrorString(hr)); } while (0)

struct dxDevice final : rhi::Device {
    dxDevice(ID3D12Device4 *pDevice): pDevice(pDevice) { }

    ~dxDevice() override {
        drop(pDevice, "device");
    }
private:
    ID3D12Device4 *pDevice;
};

struct dxAdapter final : rhi::Adapter {
    dxAdapter(IDXGIAdapter1 *pAdapter): pAdapter(pAdapter) {
        DXGI_ADAPTER_DESC1 desc;
        DX_CHECK(pAdapter->GetDesc1(&desc));

        description = engine::narrow(desc.Description);
        flags = desc.Flags;
    }

    std::string_view getName() const override {
        return description;
    }

    rhi::Adapter::Type getType() const override {
        if (flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
            return rhi::Adapter::eSoftare;
        }

        if (flags & DXGI_ADAPTER_FLAG_REMOTE) {
            return rhi::Adapter::eOther;
        }

        return rhi::Adapter::eDiscrete;
    }

    IDXGIAdapter1 *get() { return pAdapter; }

private:
    IDXGIAdapter1 *pAdapter;
    std::string description;
    UINT flags;
};

struct dxInstance final : rhi::Instance {
    dxInstance(ID3D12Debug *pDxDebug, IDXGIDebug *pDebug, IDXGIFactory4 *pFactory)
        : pDxDebug(pDxDebug)
        , pDebug(pDebug)
        , pFactory(pFactory)
    { 
        collectAdapters();
    }

    rhi::Device *newDevice(rhi::Adapter *adapter) override {
        auto *it = static_cast<dxAdapter*>(adapter);

        ID3D12Device4 *pDevice;

        DX_CHECK(D3D12CreateDevice(it->get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&pDevice)));

        return new dxDevice(pDevice);
    }

    std::span<rhi::Adapter*> getAdapters() override {
        return devices;
    }

private:
    void collectAdapters() {
        IDXGIAdapter1 *pAdapter;
        for (UINT i = 0; SUCCEEDED(pFactory->EnumAdapters1(i, &pAdapter)); i++) {
            devices.push_back(new dxAdapter(pAdapter));
        }
    }
    
    std::vector<rhi::Adapter*> devices;

    ID3D12Debug *pDxDebug;
    IDXGIDebug *pDebug;
    IDXGIFactory4 *pFactory;
};

rhi::Instance *rhi::getInstance() {
    ID3D12Debug *pDxDebug;
    IDXGIDebug *pDebug;
    IDXGIFactory4 *pFactory;

    DX_CHECK(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&pDebug)));

    UINT factoryFlags = 0;

    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&pDxDebug)))) {
        pDxDebug->EnableDebugLayer();
        factoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
    }

    DX_CHECK(CreateDXGIFactory2(factoryFlags, IID_PPV_ARGS(&pFactory)));

    return new dxInstance(pDxDebug, pDebug, pFactory);
}
