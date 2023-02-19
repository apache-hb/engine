#include "simcoe/render/graph.h"

using namespace simcoe;
using namespace simcoe::render;

namespace {
    std::string edgeName(InEdge *pEdge) {
        return std::format("in:{}:{}", pEdge->getPass()->getName(), pEdge->getName());
    }

    std::string edgeName(OutEdge *pEdge) {
        return std::format("out:{}:{}", pEdge->getPass()->getName(), pEdge->getName());
    }
}

GraphObject::GraphObject(const char *pzName, Graph& graph)
    : pzName(pzName)
    , graph(graph)
{ }

GraphObject::GraphObject(const GraphObject& other)
    : GraphObject(other.pzName, other.graph)
{ }

const char *GraphObject::getName() const {
    return pzName;
}

Graph& GraphObject::getGraph() const {
    return graph;
}

Context& GraphObject::getContext() const {
    return getGraph().getContext();
}

void InEdge::updateEdge(OutEdge *pEdge) {     
    ASSERTF(pEdge != nullptr, "null setResource on {}", edgeName(this));
    pSource = pEdge;
}

ID3D12Resource *InEdge::getResource() {
    return pSource->getResource();
}

D3D12_CPU_DESCRIPTOR_HANDLE InEdge::cpuHandle() {
    return pSource->cpuHandle();
}

D3D12_GPU_DESCRIPTOR_HANDLE InEdge::gpuHandle() {
    return pSource->gpuHandle();
}

ID3D12Resource *RelayEdge::getResource() {
    ID3D12Resource *pResource = pOther->getResource();
    ASSERTF(pResource != nullptr, "null resource returned from {} in relay {}", edgeName(pOther), edgeName(this));
    return pResource;
}

D3D12_RESOURCE_STATES RelayEdge::getState() const {
    return pOther->getState();
}

D3D12_CPU_DESCRIPTOR_HANDLE RelayEdge::cpuHandle() {
    return pOther->cpuHandle();
}

D3D12_GPU_DESCRIPTOR_HANDLE RelayEdge::gpuHandle() {
    return pOther->gpuHandle();
}

InEdge *Pass::addInput(InEdge *pEdge) {
    inputs.emplace_back(pEdge);
    return pEdge;
}

OutEdge *Pass::addOutput(OutEdge *pEdge) {
    outputs.emplace_back(pEdge);
    return pEdge;
}

Graph::Graph(Context& context)
    : context(context)
{ }

struct PassTree final {
    PassTree(Pass *pRoot)
        : pPass(pRoot) 
    { }

    void add(PassTree&& child) { children.push_back(std::move(child)); }

    Pass *pPass;
    std::vector<PassTree> children;
};

struct GraphBuilder final {
    GraphBuilder(Graph& graph, Pass *pRoot) : graph(graph) { 
        run(build(pRoot));
    }

private:
    PassTree build(Pass *pRoot) {
        ASSERT(pRoot != nullptr);
        PassTree tree(pRoot);

        auto edges = graph.getEdges();

        for (auto& input : pRoot->getInputs()) {
            ASSERTF(edges.contains(input.get()), "edge {} was not found", edgeName(input.get()));

            auto& wire = edges.at(input.get());
            tree.add(build(wire->getPass()));
        }

        return tree;
    }

    void wireBarriers(Pass *pPass) {
        std::vector<D3D12_RESOURCE_BARRIER> barriers;
        
        // for each input edge
        for (auto& dest : pPass->getInputs()) {
            // find the source of the input
            auto& source = graph.getEdges().at(dest.get());
            
            dest->updateEdge(source);

            if (source->getState() == dest->getState()) { continue; }

            barriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(
                /* pResource = */ dest->getResource(),
                /* StateBefore = */ source->getState(),
                /* StateAfter = */ dest->getState()
            ));
        }

        if (!barriers.empty()) {
            graph.getContext().getCommandList()->ResourceBarrier(
                /* NumBarriers = */ UINT(barriers.size()),
                /* pBarriers = */ barriers.data()
            );
        }
    }

    void run(const PassTree& tree) {
        auto& [pPass, deps] = tree;
        if (visited[pPass].test_and_set()) {
            return;
        }

        for (auto& dep : deps) {
            run(dep);
        }

        wireBarriers(pPass);

        pPass->execute();
    }

    std::unordered_map<Pass*, std::atomic_flag> visited;
    Graph& graph;
};

void Graph::connect(OutEdge *pSource, InEdge *pTarget) {
    ASSERT(pSource != nullptr);
    ASSERT(pTarget != nullptr);

    edges[pTarget] = pSource;
}

void Graph::start() {
    for (auto& [pzName, pPass] : passes) {
        pPass->start();
    }
}

void Graph::stop() {
    for (auto& [pzName, pPass] : passes) {
        pPass->stop();
    }
}

void Graph::execute(Pass *pRoot) {
    auto& ctx = getContext();

    ctx.begin();
    GraphBuilder graph{*this, pRoot};
    ctx.end();
}

Context& Graph::getContext() {
    return context;
}
