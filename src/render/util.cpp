#include "util.h"

#include "util/strings.h"

#include <comdef.h>
#include <d3dcompiler.h>

namespace engine::render {
    logging::Channel *render = new logging::ConsoleChannel("d3d12", stdout);

    std::string to_string(HRESULT hr) {
        _com_error err(hr);
        auto str = err.ErrorMessage();
        return strings::encode(str);
    }

    Result<Display> createDisplay(dxgi::Output output) {
        dxgi::OutputDesc desc;
        if (HRESULT hr = output->GetDesc(&desc); FAILED(hr)) {
            render->fatal("failed to get output description\n{}", to_string(hr));
            return fail(hr);
        }

        return pass(Display { output, desc });
    }

    Result<Adapter> createAdapter(dxgi::Adapter1 adapter) {
        dxgi::AdapterDesc1 desc;
        
        if (HRESULT hr = adapter->GetDesc1(&desc); FAILED(hr)) {
            render->fatal("failed to query adapter desc\n{}", to_string(hr));
            return fail(hr);
        }

        std::vector<Display> displays;

        dxgi::Output output;
        for (UINT i = 0; adapter->EnumOutputs(i, &output) != DXGI_ERROR_NOT_FOUND; i++) {
            auto display = createDisplay(output);
            if (!display) { return display.forward(); }
            displays.push_back(display.value());
        }

        return pass(Adapter{ adapter, desc, displays });
    }

#if D3D12_DEBUG
#   define FACTORY_FLAGS DXGI_CREATE_FACTORY_DEBUG
#else
#   define FACTORY_FLAGS 0
#endif

    Result<Factory> createFactory() {
        dxgi::Factory4 factory;
        if (HRESULT hr = CreateDXGIFactory2(FACTORY_FLAGS, IID_PPV_ARGS(&factory)); FAILED(hr)) {
            render->fatal("failed to create dxgi factory\n{}", to_string(hr));
            return fail(hr);
        }

        BOOL tearing = false;
        if (auto result = factory.as<dxgi::Factory5>(); result.valid()) {
            auto it = result.value();
            if (HRESULT hr = it->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &tearing, sizeof(tearing)); FAILED(hr)) {
                render->warn("failed to query tearing support\n{}", to_string(hr));
            }
            it.release();
        }

        std::vector<Adapter> adapters;
        dxgi::Adapter1 adapter;
        for (UINT i = 0; factory->EnumAdapters1(i, adapter.addr()) != DXGI_ERROR_NOT_FOUND; i++) {
            auto result = createAdapter(adapter);
            if (!result) { return result.forward(); }
            adapters.push_back(result.value());
        }

        render->info("{} adapters found", adapters.size());
        render->info("tearing {}", tearing ? "supported" : "unsupported");

        return pass(Factory{ factory, adapters, tearing != FALSE });
    }

    namespace debug {
        d3d12::Debug d3d12debug;
        dxgi::Debug dxgidebug;

        HRESULT enable() {
            if (HRESULT hr = DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgidebug)); FAILED(hr)) {
                render->fatal("failed to get dxgi debug interface\n{}", to_string(hr));
                return hr;
            }

            if (HRESULT hr = D3D12GetDebugInterface(IID_PPV_ARGS(&d3d12debug)); FAILED(hr)) {
                render->fatal("failed to get d3d12 debug interface\n{}", to_string(hr));
                return hr;
            }

            d3d12debug->EnableDebugLayer();
            render->info("debug layer enabled");
            return S_OK;
        }

        HRESULT report() {
            render->info("reporting all live objects");
            return dxgidebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
        }

        void disable() {
            d3d12debug.drop("debug-layer");
            render->info("debug layer disabled");
        }
    }

#if D3D12_DEBUG
#   define D3D_COMPILE_FLAGS D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION
#else
#   define D3D_COMPILE_FLAGS 0
#endif

    namespace d3d {
        HRESULT compileFromFile(LPCWSTR path, LPCSTR entry, LPCSTR shaderModel, ID3DBlob **blob) {
            d3d::Blob error;
            HRESULT hr = D3DCompileFromFile(path, nullptr, nullptr, entry, shaderModel, D3D_COMPILE_FLAGS, 0, blob, &error);

            if (FAILED(hr)) {
                render->fatal("failed to compile shader {}\n{}\n{}", entry, to_string(hr), !!error ? (char*)error->GetBufferPointer() : "null-error");
            }

            return hr;
        }
    }
}
