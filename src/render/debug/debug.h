#pragma once

namespace engine::render::debug {
    void enable();
    void disable();
    void report();
}

#pragma region debug configuration macros

#define ALWAYS_SHADER_FLAGS D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_WARNINGS_ARE_ERRORS

#if D3D12_DEBUG
#   define DEFAULT_FACTORY_FLAGS DXGI_CREATE_FACTORY_DEBUG
#   define DEFAULT_SHADER_FLAGS ALWAYS_SHADER_FLAGS | D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION
#else
#   define DEFAULT_FACTORY_FLAGS 0
#   define DEFAULT_SHADER_FLAGS ALWAYS_SHADER_FLAGS
#endif
