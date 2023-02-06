#include "simcoe/render/graph.h"

using namespace simcoe;
using namespace simcoe::render;

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

Wire *Pass::addInput(Wire *pWire) {
    inputs.emplace_back(pWire);
    return pWire;
}

Wire *Pass::addOutput(Wire *pWire) {
    inputs.emplace_back(pWire);
    return pWire;
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

        for (auto& pWire : pRoot->inputs) {
            auto& wire = graph.wires.at(pWire.get());
            tree.add(build(wire->pPass));
        }

        return tree;
    }

    void wireBarriers(Pass *pPass) {
        std::vector<D3D12_RESOURCE_BARRIER> barriers;
        
        for (auto& source : pPass->inputs) {
            auto& target = graph.wires.at(source.get());
            
            if (source->state == target->state) { continue; }

            ID3D12Resource *pResource = graph.wireResources.at(source.get())->pResource;

            ASSERT(pResource != nullptr);

            barriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(
                /* pResource = */ pResource,
                /* StateBefore = */ source->state,
                /* StateAfter = */ target->state
            ));
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

void Graph::connect(Wire *pSource, Wire *pTarget) {
    ASSERT(pSource != nullptr);
    ASSERT(pTarget != nullptr);

    wires[pTarget] = pTarget;
}

void Graph::connect(Wire *pWire, Resource *pResource) {
    ASSERT(pWire != nullptr);
    ASSERT(pResource != nullptr);

    wireResources[pWire] = pResource;
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
    context.begin();
    GraphBuilder graph{*this, pRoot};
    context.end();
}

Context& Graph::getContext() {
    return context;
}
