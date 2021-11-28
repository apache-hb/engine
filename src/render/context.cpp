#include "context.h"

extern "C" { 
    __declspec(dllexport) extern const UINT D3D12SDKVersion = 4;
    __declspec(dllexport) extern const auto* D3D12SDKPath = u8".\\D3D12\\"; 
}

namespace render {
    namespace {
        dxgi::Adapter1 getAdapter(dxgi::Factory4 factory, UINT idx) {
            dxgi::Adapter1 result;
            if (factory->EnumAdapters1(idx, &result) == DXGI_ERROR_NOT_FOUND) {
                return nullptr;
            }
            return result;
        }

        void infoQueueCallback(D3D12_MESSAGE_CATEGORY category, D3D12_MESSAGE_SEVERITY severity, D3D12_MESSAGE_ID id, LPCSTR msg, void *ctx) {
            fprintf(stderr, "d3d12: %s\n", msg);
        }
    }

    HRESULT Context::create(const Instance &instance, size_t index) { 
        factory = instance.factory;
        adapter = instance.adapters[index];

        if (HRESULT hr = D3D12CreateDevice(adapter.get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device)); FAILED(hr)) {
            fprintf(stderr, "d3d12: D3D12CreateDevice = 0x%x\n", hr);
            return hr;
        }

        if ((infoQueue = device.as<d3d12::InfoQueue1>(L"d3d12-info-queue")).valid()) {
            DWORD cookie = 0;
            if (HRESULT hr = infoQueue->RegisterMessageCallback(infoQueueCallback, D3D12_MESSAGE_CALLBACK_FLAG_NONE, nullptr, &cookie); FAILED(hr)) {
                fprintf(stderr, "d3d12: RegisterMessageCallback = 0x%x\n", hr);
            }
        }

        D3D12_COMMAND_QUEUE_DESC desc = { .Type = D3D12_COMMAND_LIST_TYPE_DIRECT };
        if (HRESULT hr = device->CreateCommandQueue(&desc, IID_PPV_ARGS(&commandQueue)); FAILED(hr)) {
            fprintf(stderr, "d3d12: CreateCommandQueue = 0x%x\n", hr);
            return hr;
        }

        return S_OK;
    }

    void Context::destroy() {
        commandQueue.drop();
        if (infoQueue.valid()) { 
            infoQueue.release(); 
        }
        device.drop();
    }

    HRESULT Instance::create() {
        if (HRESULT hr = D3D12GetDebugInterface(IID_PPV_ARGS(&debug)); SUCCEEDED(hr)) {
            debug->EnableDebugLayer();
        } else {
            fprintf(stderr, "d3d12: D3D12GetDebugInstance = 0x%x\n", hr);
            return hr;
        }

        if (HRESULT hr = CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&factory)); SUCCEEDED(hr)) {
            dxgi::Adapter1 adapter;
            for (UINT i = 0; (adapter = getAdapter(factory, i)).valid(); i++) {
                adapters.push_back(adapter);
            }
        } else {
            fprintf(stderr, "d3d12: CreateDXGIFactory2 = 0x%x\n", hr);
            return hr;
        }

        return S_OK;
    }

    void Instance::destroy() {
        for (auto &adapter : adapters) { adapter.drop(); }
        factory.drop();
        debug.drop();
    }
}   
