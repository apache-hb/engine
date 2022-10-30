#include "engine/graph/pass.h"
#include "engine/graph/render.h"

using namespace simcoe;
using namespace simcoe::graph;

namespace {
    template<typename T, typename... A>
    T *newResource(RenderGraph *graph, const char *name, A&&... args) {
        return graph->newResource<T>(name, std::forward<A>(args)...);
    }
}

void DirectPass::init() {
    commands = newResource<CommandResource>(getParent(), "direct", rhi::CommandList::Type::eDirect);
}
