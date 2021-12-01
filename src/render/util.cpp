#include "util.h"

#include "util/strings.h"

namespace engine::render {
    engine::logging::Channel *render = new engine::logging::ConsoleChannel("d3d12", stdout);

    namespace debug {
        d3d12::Debug debugger;
        UINT flags = 0;

        void start() {
            render->info("enabling debugger");
            if (HRESULT hr = D3D12GetDebugInterface(IID_PPV_ARGS(&debugger)); FAILED(hr)) {
                render->warn("failed to enable debug layer");
            } else {
                debugger->EnableDebugLayer();
                flags = DXGI_CREATE_FACTORY_DEBUG;
            }
        }

        void end() {
            render->info("disabling debugger");
            debugger.tryDrop("debugger");
        }
    }

    void ensure(HRESULT result, std::string_view message) {
        render->ensure(SUCCEEDED(result), message, [&] {
            throw Error(result, std::string(message));
        });
    }

    std::string Error::string() const {
        return strings::cformat("(hresult: 0x%x, message: %s)", hresult(), msg().data());
    }

    Adapter::Name Adapter::ID::name() const {
        return std::format("(low: {}, high: {})", LowPart, HighPart);
    }

    Adapter::Adapter(dxgi::Adapter1 adapter) : adapter(adapter) {
        ensure(adapter->GetDesc1(&desc), "get-desc1");
    }

    Adapter::Name Adapter::name() const { 
        // adapter names are stored as wstrings
        // we need a string to work with the rest 
        // of the engine
        Adapter::Name result;
        auto *cursor = desc.Description;
        while (*cursor) { result += *cursor++; }
        return result;
    }

    Adapter::MemorySize Adapter::video() const { return desc.DedicatedVideoMemory; }
    Adapter::MemorySize Adapter::system() const { return desc.DedicatedSystemMemory; }
    Adapter::MemorySize Adapter::shared() const { return desc.SharedSystemMemory; }
    Adapter::ID Adapter::luid() const { return Adapter::ID(desc.AdapterLuid); }

    Factory::Factory() {
        ensure(CreateDXGIFactory2(debug::flags, IID_PPV_ARGS(&factory)), "create-dxgi-factory");
        
        dxgi::Adapter1 adapter;
        
        for (UINT i = 0; factory->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND; i++) {
            all.push_back(Adapter(adapter));
        }
    }

    std::span<Adapter> Factory::adapters() { return all; }
}
