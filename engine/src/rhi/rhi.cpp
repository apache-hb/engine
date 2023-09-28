#include "simcoe/rhi/rhi.h"
#include "simcoe/core/system.h"

namespace rhi = simcoe::rhi;

rhi::Barrier rhi::transition(rhi::ISurface *pResource, ResourceState from, ResourceState to) {
    return rhi::Barrier {
        .type = rhi::BarrierType::eTransition,
        .data = { .transition = { .pResource = pResource, .from = from, .to = to } }
    };
}
