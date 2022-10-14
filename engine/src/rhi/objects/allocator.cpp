#include "engine/rhi/rhi.h"
#include "objects/common.h"

using namespace simcoe;

void rhi::Allocator::reset() {
    DX_CHECK(get()->Reset());
}
