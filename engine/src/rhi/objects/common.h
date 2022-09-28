#pragma once

#include "engine/base/panic.h"
#include "engine/base/util.h"
#include "engine/rhi/rhi.h"

#include <type_traits>

#include "dx/d3dx12.h"

#include <dxgi1_6.h>
#include <dxgidebug.h>

namespace engine {
    template<typename T>
    concept IsComObject = std::is_base_of_v<IUnknown, T>;
    
    template<IsComObject T>
    struct ComDeleter {
        void operator()(T *ptr) {
            ptr->Release();
        }
    };

    template<IsComObject T>
    using UniqueComPtr = UniquePtr<T, ComDeleter<T>>;

    std::string hrErrorString(HRESULT hr);

    extern IDXGIFactory5 *gFactory;
    extern ID3D12Debug *gDxDebug;
    extern IDXGIDebug *gDebug;

    constexpr DXGI_FORMAT getElementFormat(rhi::Format format) {
        switch (format) {
        case rhi::Format::uint32: return DXGI_FORMAT_R32_UINT;
        
        case rhi::Format::float32: return DXGI_FORMAT_R32_FLOAT;
        case rhi::Format::float32x2: return DXGI_FORMAT_R32G32_FLOAT;
        case rhi::Format::float32x3: return DXGI_FORMAT_R32G32B32_FLOAT;
        case rhi::Format::float32x4: return DXGI_FORMAT_R32G32B32A32_FLOAT;
        
        default: return DXGI_FORMAT_UNKNOWN;
        }
    }
}

#define DX_CHECK(expr) do { HRESULT hr = (expr); ASSERTF(SUCCEEDED(hr), #expr " => {}", engine::hrErrorString(hr)); } while (0)
