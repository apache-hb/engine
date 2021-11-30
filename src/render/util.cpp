#include "util.h"

#include <format>

namespace engine::render {
    void ensure(HRESULT result, std::string message) {
        if (FAILED(result)) { throw new Error(result, message); }
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
        ensure(CreateDXGIFactory2(0, IID_PPV_ARGS(&factory)), "create-dxgi-factory");
        
        dxgi::Adapter1 adapter;
        
        for (UINT i = 0; factory->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND; i++) {
            all.push_back(Adapter(adapter));
        }
    }

    std::span<Adapter> Factory::adapters() { return all; }
}
