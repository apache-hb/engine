#include "render/objects/resource.h"

using namespace engine::render;

D3D12_GPU_VIRTUAL_ADDRESS Resource::gpuAddress() {
    return get()->GetGPUVirtualAddress();
}
