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

Graph& GraphObject::getGraph() {
    return graph;
}

Context& GraphObject::getContext() {
    return getGraph().getContext();
}

void Edge::setResource(ID3D12Resource*) {
    ASSERTF(false, "invalid setResource on {}", getName());
}

void InEdge::setResource(ID3D12Resource *pIn) { 
    ASSERTF(pIn != nullptr, "null setResource on {}", edgeName(this));
    pResource = pIn;
}

ID3D12Resource *InEdge::getResource() {
    return pResource;
}

ID3D12Resource *RelayEdge::getResource() {
    ID3D12Resource *pResource = pOther->getResource();
    ASSERTF(pResource != nullptr, "null resource returned from {} in relay {}", edgeName(pOther), edgeName(this));
    return pResource;
}

D3D12_RESOURCE_STATES RelayEdge::getState() const {
    return pOther->getState();
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

        gRenderLog.info("building pass for {}", pRoot->getName());

        for (auto& input : pRoot->getInputs()) {
            ASSERTF(edges.find(input.get()) != edges.end(), "edge {} was not found", edgeName(input.get()));

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
            
            ID3D12Resource *pResource = source->getResource();
            dest->setResource(pResource);

            ASSERTF(pResource != nullptr, "resource for {} was not found", edgeName(dest.get()));

            if (source->getState() == dest->getState()) { continue; }

            barriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(
                /* pResource = */ pResource,
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

        gRenderLog.info("executing pass {}", pPass->getName());

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
    GraphBuilder graph{*this, pRoot};
}

Context& Graph::getContext() {
    return context;
}
