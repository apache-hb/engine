#pragma once

#include <d3dcompiler.h>
#include <dxgidebug.h>
#include <dxgi1_6.h>

#define ALWAYS_SHADER_FLAGS D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_WARNINGS_ARE_ERRORS

#if D3D12_DEBUG
#   define DEFAULT_FACTORY_FLAGS DXGI_CREATE_FACTORY_DEBUG
#   define DEFAULT_SHADER_FLAGS ALWAYS_SHADER_FLAGS | D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION
#else
#   define DEFAULT_FACTORY_FLAGS 0
#   define DEFAULT_SHADER_FLAGS ALWAYS_SHADER_FLAGS
#endif

namespace engine::render {
    namespace debug {
        void enable();
        void disable();
        void report();
    }

    constexpr auto kFactoryFlags = DEFAULT_FACTORY_FLAGS;
    constexpr auto kShaderFlags = DEFAULT_SHADER_FLAGS;
}
