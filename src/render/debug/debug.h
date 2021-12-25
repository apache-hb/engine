#pragma once

namespace engine::render::debug {
    void enable();
    void disable();
    void report();
}

#pragma region debug configuration macros

#if D3D12_DEBUG
#   define DEFAULT_FACTORY_FLAGS DXGI_CREATE_FACTORY_DEBUG
#else
#   define DEFAULT_FACTORY_FLAGS 0
#endif
