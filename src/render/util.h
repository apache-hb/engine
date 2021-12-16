#pragma once

#include "util/error.h"
#include "util/units.h"
#include "logging/log.h"
#include "d3dx12.h"
#include <dxgi1_6.h>
#include <vector>
#include <span>
#include <DirectXMath.h>

#define cbuffer struct __declspec(align(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT))

namespace engine::render {
    using namespace DirectX;

    std::string to_string(HRESULT hr);

    struct Error : engine::Error {
        Error(HRESULT hr, std::string message = "", std::source_location location = std::source_location::current())
            : engine::Error(message, location)
            , code(hr)
        { }

        HRESULT error() const { return code; }

        virtual std::string query() const override {
            return std::format("hresult: {}\n{}", to_string(code), what());
        }
    private:
        HRESULT code;
    };

    void check(HRESULT hr, const std::string& message = "", std::source_location location = std::source_location::current());

    template<typename T>
    struct Com {
        using Self = T;
        Com(T *self = nullptr) : self(self) { }

        static Com<T> invalid() { return Com<T>(nullptr); }

        void rename(std::wstring_view name) {
            self->SetName(name.data());
        }

        operator bool() const { return self != nullptr; }
        bool valid() const { return self != nullptr; }

        T *operator->() { return self; }
        T *operator*() { return self; }
        T **operator&() { return &self; }

        T *get() { return self; }
        T **addr() { return &self; }

        void drop(std::string_view name = "") {
            auto refs = release();
            if (refs > 0) {
                log::render->fatal("failed to drop {}, {} references are still held", name, refs);
            }
        }

        void tryDrop(std::string_view name = "") {
            if (!valid()) { return; }
            drop(name);
        }

        auto release() { return self->Release(); }

        template<typename O>
        Com<O> as() {
            O *other = nullptr;
            if (HRESULT hr = self->QueryInterface(IID_PPV_ARGS(&other)); FAILED(hr)) {
                return Com<O>::invalid();
            }
            return Com<O>(other);
        }
    private:
        T *self;
    };

    struct Output {
        Output(IDXGIOutput *output = nullptr);

        std::string name() const;
        RECT coords() const;

    private:
        Com<IDXGIOutput> self;
        DXGI_OUTPUT_DESC desc;
    };

    struct Adapter {
        Adapter(IDXGIAdapter1 *adapter = nullptr);

        std::string name() const;
        units::Memory videoMemory() const;
        units::Memory systemMemory() const;
        units::Memory sharedMemory() const;
        std::span<Output> outputs();
        bool software() const;

        auto get() { return self.get(); }

    private:
        Com<IDXGIAdapter1> self;
        DXGI_ADAPTER_DESC1 desc;
        std::vector<Output> displays;
    };

    struct Factory {
        Factory(bool debug = false);
        
        std::span<Adapter> adapters();
        Adapter adapter(size_t index);
        UINT presentFlags() const;

        Com<IDXGISwapChain1> createSwapChain(Com<ID3D12CommandQueue> queue, HWND window, const DXGI_SWAP_CHAIN_DESC1 &desc);

    private:
        Com<IDXGIFactory4> self;
        std::vector<Adapter> devices;
        BOOL tearing = false;
    };

    Com<ID3DBlob> compileShader(std::wstring_view path, std::string_view entry, std::string_view target);

    namespace d3d12 {
        struct Scissor : D3D12_RECT {
            using Super = D3D12_RECT;

            Scissor(LONG width, LONG height)
                : Super({ 0, 0, width, height })
            { }
        };

        struct Viewport : D3D12_VIEWPORT {
            using Super = D3D12_VIEWPORT;
            Viewport(FLOAT width, FLOAT height)
                : Super({ 0.f, 0.f, width, height, 0.f, 1.f })
            { }
        };
    }

    struct Vertex {
        XMFLOAT3 position;
        XMFLOAT4 colour;
    };
}
