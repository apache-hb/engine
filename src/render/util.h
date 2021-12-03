#pragma once

#include <vector>
#include <string>
#include <span>

#include "util/error.h"
#include "util/units.h"
#include "util/win32.h"
#include "logging/log.h"

#include <dxgi1_6.h>
#include "d3dx12.h"
#include <d3d12sdklayers.h>

namespace engine::render {
    extern engine::logging::Channel *render;

    void ensure(HRESULT result, std::string_view message);
    std::string name(HRESULT hresult);

    struct Error : engine::Error {
        using Super = engine::Error;

        Error(HRESULT error, std::string message)
            : Super(message)
            , error(error)
        { }

        HRESULT hresult() const { return error; }
    
        virtual std::string string() const override;

    private:
        HRESULT error;
    };

    template<typename T>
    struct Com {
        using Self = T;

        Com(T *self = nullptr) : self(self) { }

        T *operator->() { return get(); }
        T *operator*() { return get(); }
        T **operator&() { return addr(); }

        bool valid() const { return self != nullptr; }
        operator bool() const { return valid(); }

        T *get() { return self; }
        T **addr() { return &self; }

        template<typename O>
        O as() {
            using Other = typename O::Self;
            Other *result = nullptr;
            if (HRESULT hr = self->QueryInterface(IID_PPV_ARGS(&result)); FAILED(hr)) {
                render->warn("failed to cast object {}", name(hr));
            }
            return O(result);
        }

        ULONG release() {
            return self->Release();
        }

        void drop(std::string_view id = "") {
            auto refs = release();
            render->ensure(refs == 0,
                std::format("failed to drop {} object, {} references remain", id, refs),
                [&] { throw engine::Error(std::format("cannot drop {} because it still has {} references", id, refs)); }
            );
        }

        void tryDrop(std::string_view id = "") {
            if (valid()) { drop(id); }
        }

    private:
        T *self;
    };

    namespace dxgi {
        using Factory4 = Com<IDXGIFactory4>;
        using Adapter1 = Com<IDXGIAdapter1>;
        using AdapterDesc1 = DXGI_ADAPTER_DESC1;
    
        using SwapChain1 = Com<IDXGISwapChain1>;
        using SwapChain3 = Com<IDXGISwapChain3>;
    }

    namespace d3d12 {
        using Debug = Com<ID3D12Debug>;
        using Device1 = Com<ID3D12Device1>;
        using InfoQueue1 = Com<ID3D12InfoQueue1>;
        using CommandList = Com<ID3D12CommandList>;
        using CommandQueue = Com<ID3D12CommandQueue>;
        using DescriptorHeap = Com<ID3D12DescriptorHeap>;
        using Resource = Com<ID3D12Resource>;
        using CommandAllocator = Com<ID3D12CommandAllocator>;
    }

    namespace d3d {
        using FeatureLevel = D3D_FEATURE_LEVEL;
    }

    struct Adapter {
        using Name = std::string;
        using MemorySize = units::Memory;

        struct ID : LUID {
            Name name() const;
        };

        Adapter(dxgi::Adapter1 adapter);

        Name name() const;
        MemorySize video() const;
        MemorySize system() const;
        MemorySize shared() const;
        ID luid() const;

        auto get() { return adapter.get(); }

        d3d12::Device1 createDevice(d3d::FeatureLevel level);

    private:
        dxgi::Adapter1 adapter;
        dxgi::AdapterDesc1 desc;
    };

    struct Factory {
        Factory();

        std::span<Adapter> adapters();
        auto operator->() { return factory.get(); }

    private:
        std::vector<Adapter> all;
        dxgi::Factory4 factory;
    };

    namespace debug {
        void start();
        void end();
    }
}
