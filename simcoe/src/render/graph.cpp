#include "simcoe/render/graph.h"

using namespace simcoe;
using namespace simcoe::render;

GraphObject::GraphObject(const char *pzName, Graph& graph)
    : pzName(pzName)
    , graph(graph)
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

Graph::Graph(Context& context)
    : context(context)
{ }

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
    pRoot->execute(); // TODO: lol
}

Context& Graph::getContext() {
    return context;
}
