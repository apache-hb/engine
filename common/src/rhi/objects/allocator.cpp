#include "engine/rhi/rhi.h"
#include "objects/common.h"

using namespace simcoe;

void rhi::Allocator::reset() {
    HR_CHECK(get()->Reset());
}
