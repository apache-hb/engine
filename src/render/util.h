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

    std::string toString(HRESULT hr);

    struct Error : engine::Error {
        Error(HRESULT hr, std::string message = "", std::source_location location = std::source_location::current())
            : engine::Error(message, location)
            , code(hr)
        { }

        HRESULT error() const { return code; }

        virtual std::string query() const override {
            return std::format("hresult: {}\n{}", toString(code), what());
        }
    private:
        HRESULT code;
    };

    void check(HRESULT hr, const std::string& message = "", std::source_location location = std::source_location::current());

    template<typename T>
    struct Com {
        using Self = T;
        Com(T *self = nullptr) : self(self) { }
        Com& operator=(const Com&) = default;
        Com& operator=(T *self) { this->self = self; return *this; }

        static Com<T> invalid() { return Com<T>(nullptr); }

        void rename(std::wstring_view name) {
            self->SetName(name.data());
        }

        void set(Com<T> other) {
            self = other.get();
        }

        operator bool() const { return self != nullptr; }
        bool valid() const { return self != nullptr; }

        T *operator->() { return self; }
        T *operator*() { return self; }
        T **operator&() { return &self; }

        T *get() { return self; }
        T **addr() { return &self; }

        const T *get() const { return self; }
        const T **addr() const { return &self; }

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

    Com<ID3DBlob> compileShader(std::wstring_view path, std::string_view entry, std::string_view target);

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

    struct View {
        View() = default;
        View(LONG width, LONG height)
            : scissor({ width, height })
            , viewport({ FLOAT(width), FLOAT(height) })
        { }

        Scissor scissor{0, 0};
        Viewport viewport{0.f, 0.f};
    };

    using Colour = XMFLOAT4;

    namespace colour {
        constexpr Colour red = { 1.f, 0.f, 0.f, 1.f };
        constexpr Colour orange = { 1.f, 0.5f, 0.f, 1.f };
        constexpr Colour yellow = { 1.f, 1.f, 0.f, 1.f };
        constexpr Colour green = { 0.f, 1.f, 0.f, 1.f };
        constexpr Colour blue = { 0.f, 0.f, 1.f, 1.f };
        constexpr Colour indigo = { 0.f, 0.f, 1.f, 1.f };
        constexpr Colour violet = { 1.f, 0.f, 1.f, 1.f };
        constexpr Colour white = { 1.f, 1.f, 1.f, 1.f };
    }

    cbuffer ConstBuffer {
        XMFLOAT4X4 model;
        XMFLOAT4X4 view;
        XMFLOAT4X4 projection;
    };
}
