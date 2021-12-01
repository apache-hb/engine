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
        Com(T *self = nullptr) : self(self) { }

        T *operator->() { return get(); }
        T *operator*() { return get(); }
        T **operator&() { return addr(); }

        bool valid() const { return self != nullptr; }
        operator bool() const { return valid(); }

        T *get() { return self; }
        T **addr() { return &self; }

        ULONG release() {
            return self->Release();
        }

        void drop(std::string_view name = "") {
            auto refs = release();
            render->check(refs == 0, engine::Error(std::format("cannot drop {} because it still has {} references", name, refs)));
        }

        void tryDrop(std::string_view name = "") {
            if (valid()) { drop(name); }
        }

    private:
        T *self;
    };

    namespace dxgi {
        using Factory4 = Com<IDXGIFactory4>;
        using Adapter1 = Com<IDXGIAdapter1>;
        using AdapterDesc1 = DXGI_ADAPTER_DESC1;
    
        using SwapChain1 = Com<IDXGISwapChain1>;
    }

    namespace d3d12 {
        using Debug = Com<ID3D12Debug>;
        using Device1 = Com<ID3D12Device1>;
        using CommandList = Com<ID3D12CommandList>;
        using CommandQueue = Com<ID3D12CommandQueue>;
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
