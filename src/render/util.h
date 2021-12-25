#pragma once

#include "util/error.h"
#include "util/units.h"
#include "util/strings.h"
#include "logging/log.h"

#include <d3dx12.h>
#include <dxgi1_6.h>

#include <vector>
#include <span>
#include <concepts>
#include <type_traits>

#define cbuffer struct __declspec(align(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT))

namespace engine::render {
#pragma region helper functions and error handling

    std::string toString(HRESULT hr);

    void check(HRESULT hr, std::string_view message = "", std::source_location location = std::source_location::current());
    void check(HRESULT hr, std::wstring_view message = L"", std::source_location location = std::source_location::current());

    struct Error : engine::Error {
        Error(HRESULT hr, std::string message = "", std::source_location location = std::source_location::current())
            : engine::Error(message, location)
            , code(hr)
        { }

        HRESULT error() const { return code; }

        virtual std::string query() const override;
    private:
        HRESULT code;
    };



#pragma region concepts

    template<typename T>
    concept IsComObject = std::is_convertible_v<T*, IUnknown*>;

    template<typename T>
    concept IsD3D12Object = std::is_convertible_v<T*, ID3D12Object*>;



#pragma region com and object wrappers

    template<IsComObject T>
    struct Com {
        using Self = T;
        Com(T *self = nullptr) : self(self) { }
        Com& operator=(const Com&) = default;
        Com& operator=(T *self) { this->self = self; return *this; }

        static Com<T> invalid() { return Com<T>(nullptr); }

        void set(Com<T> other) {
            self = other.get();
        }

        operator bool() const { return self != nullptr; }
        bool valid() const { return self != nullptr; }

        T* operator->() { return self; }
        T* operator*() { return self; }
        T** operator&() { return &self; }

        const T* operator->() const { return self; }

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

        template<IsComObject O>
        Com<O> as() {
            O *other = nullptr;
            
            if (HRESULT hr = self->QueryInterface(IID_PPV_ARGS(&other)); FAILED(hr)) {
                return Com<O>::invalid();
            }

            release();
            return Com<O>(other);
        }
    private:
        T *self;
    };

    template<IsD3D12Object T>
    struct Object : Com<T> {
        using Super = Com<T>;
        using Super::Super;

        void rename(std::wstring_view name) {
            Super::get()->SetName(name.data());
        }

        void rename(std::string_view name) {
            rename(strings::widen(name));
        }
    };



#pragma region scissor and viewport managment
    
    struct Scissor : D3D12_RECT {
        using Super = D3D12_RECT;

        Scissor(LONG top, LONG left, LONG width, LONG height)
            : Super({ top, left, width, height })
        { }
    };

    struct Viewport : D3D12_VIEWPORT {
        using Super = D3D12_VIEWPORT;
        Viewport(FLOAT top, FLOAT left, FLOAT width, FLOAT height)
            : Super({ top, left, width, height, 0.f, 1.f })
        { }
    };

    struct View {
        View() = default;
        View(FLOAT top, FLOAT left, FLOAT width, FLOAT height)
            : scissor(LONG(top), LONG(left), LONG(width + left), LONG(height + top))
            , viewport(top, left, width, height)
        { }

        Scissor scissor{0, 0, 0, 0};
        Viewport viewport{0.f, 0.f, 0.f, 0.f};
    };

    struct Resolution {
        UINT width;
        UINT height;

        float aspectRatio() const {
            return float(width) / float(height);
        }
    };
}
