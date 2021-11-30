#pragma once

#include <vector>
#include <string>
#include <span>

#include "util/error.h"
#include "util/units.h"

#include <dxgi1_6.h>
#include "d3dx12.h"

namespace engine::render {
    void ensure(HRESULT result, std::string message);

    struct Error : engine::Error {
        using Super = engine::Error;

        Error(HRESULT error, std::string message)
            : Super(message)
            , error(error)
        { }

        HRESULT hresult() const { return error; }
    
    private:
        HRESULT error;
    };

    template<typename T>
    struct Com {
        Com(T *self = nullptr) : self(self) { }

        T *operator->() { return get(); }
        T *operator*() { return get(); }
        T **operator&() { return addr(); }

        bool valid() const { return get() != nullptr; }
        operator bool() const { return valid(); }

        T *get() { return self; }
        T **addr() { return &self; }

    private:
        T *self;
    };

    namespace dxgi {
        using Factory4 = Com<IDXGIFactory4>;
        using Adapter1 = Com<IDXGIAdapter1>;
        using AdapterDesc1 = DXGI_ADAPTER_DESC1;
    }

    namespace d3d12 {
        using Device1 = Com<ID3D12Device1>;
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

    private:
        dxgi::Adapter1 adapter;
        dxgi::AdapterDesc1 desc;
    };

    struct Factory {
        Factory();

        std::span<Adapter> adapters();

    private:
        std::vector<Adapter> all;
        dxgi::Factory4 factory;
    };
}
