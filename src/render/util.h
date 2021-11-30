#pragma once

#include <vector>
#include <string>

#include "util/units.h"

#include <dxgi1_6.h>
#include "d3dx12.h"

namespace render {
    void ensure(HRESULT result, std::string message);

    struct Error {
        Error(HRESULT error, std::string message)
            : error(error)
            , message(message)
        { }

        HRESULT err() const { return error; }
        const std::string &msg() const { return message; }

    private:
        HRESULT error;
        std::string message;
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
            std::string name() const;
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

        std::vector<Adapter> adapters();

    private:
        dxgi::Factory4 factory;
    };
}
