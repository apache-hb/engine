#include "simcoe/render/graph.h"
#include "dx/d3d12.h"

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

GraphObject::GraphObject(const std::string& name, Graph& graph)
    : name(name)
    , graph(graph)
{ }

GraphObject::GraphObject(const GraphObject& other)
    : GraphObject(other.name, other.graph)
{ }

const char *GraphObject::getName() const {
    return name.c_str();
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
    ASSERTF(pSource != nullptr, "null getResource on {}", edgeName(this));
    return pSource->getResource();
}

D3D12_CPU_DESCRIPTOR_HANDLE InEdge::cpuHandle(D3D12_DESCRIPTOR_HEAP_TYPE type) {
    return pSource->cpuHandle(type);
}

D3D12_GPU_DESCRIPTOR_HANDLE InEdge::gpuHandle(D3D12_DESCRIPTOR_HEAP_TYPE type) {
    return pSource->gpuHandle(type);
}

ID3D12Resource *RelayEdge::getResource() {
    ID3D12Resource *pResource = pOther->getResource();
    ASSERTF(pResource != nullptr, "null resource returned from {} in relay {}", edgeName(pOther), edgeName(this));
    return pResource;
}

D3D12_RESOURCE_STATES RelayEdge::getState() const {
    return pOther->getState();
}

D3D12_CPU_DESCRIPTOR_HANDLE RelayEdge::cpuHandle(D3D12_DESCRIPTOR_HEAP_TYPE type) {
    return pOther->cpuHandle(type);
}

D3D12_GPU_DESCRIPTOR_HANDLE RelayEdge::gpuHandle(D3D12_DESCRIPTOR_HEAP_TYPE type) {
    return pOther->gpuHandle(type);
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

struct GraphBuilder final {
    GraphBuilder(Graph& graph, Pass *pStart, ID3D12GraphicsCommandList* pCommands) : graph(graph) { 
        run(pStart, pCommands);
    }

private:
    void wireBarriers(Pass *pPass, ID3D12GraphicsCommandList *pCommands) {
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
            pCommands->ResourceBarrier(
                /* NumBarriers = */ UINT(barriers.size()),
                /* pBarriers = */ barriers.data()
            );
        }
    }

    void run(Pass *pPass, ID3D12GraphicsCommandList* pCommands) {
        if (visited[pPass].test_and_set()) {
            return;
        }

        wireBarriers(pPass, pCommands);
        pPass->execute(pCommands);

        graph.applyToChildren(pPass, [&](Pass *pChild) {
            run(pChild, pCommands);
        });
    }

    std::unordered_map<Pass*, std::atomic_flag> visited;
    Graph& graph;
};

void Graph::wire(OutEdge *pSource, InEdge *pTarget) {
    ASSERT(pSource != nullptr);
    ASSERT(pTarget != nullptr);

    edges[pTarget] = pSource;
}

void Graph::connect(Pass *pParent, Pass *pChild) {
    ASSERT(pParent != nullptr);
    ASSERT(pChild != nullptr);

    control[pParent].insert(pChild);
}

void Graph::start() {
    commands = context.newCommandBuffer(D3D12_COMMAND_LIST_TYPE_DIRECT);
    
    for (auto& [pzName, pPass] : passes) {
        pPass->start(commands.pCommandList);
    }

    context.submitDirectCommands(commands);
}

void Graph::stop() {
    for (auto& [pzName, pPass] : passes) {
        pPass->stop();
    }
}

void Graph::execute(Pass *pStart) {
    // TODO: track effects somehow
    ID3D12DescriptorHeap *ppHeaps[] = { context.getCbvHeap().getHeap() };
    commands.pCommandList->SetDescriptorHeaps(UINT(std::size(ppHeaps)), ppHeaps);

    GraphBuilder graph{*this, pStart, commands.pCommandList};
    context.submitDirectCommands(commands);
    context.present();
}

Context& Graph::getContext() const {
    return context;
}
