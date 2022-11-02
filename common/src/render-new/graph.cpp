#include "engine/render-new/graph.h"

using namespace simcoe;
using namespace simcoe::render;

namespace {
    struct Tree {
        Tree(Pass *pass) : pass(pass) { }

        void add(Tree dep) {
            deps.push_back(std::move(dep));
        }

        Pass *pass;
        std::vector<Tree> deps;
    };

    struct GraphBuilder {

        Tree build(Graph& graph, Pass *root) {
            if (root == nullptr) { return Tree(nullptr); }
            Tree tree{root};

            for (auto& input : root->inputs) {
                auto& wire = graph.wires.at(input.get());
                tree.add(build(graph, wire->getPass()));
            }

            return tree;
        }

        void execute(Context& ctx, Graph& graph, Tree& tree) {
            auto& [pass, deps] = tree;
            if (pass == nullptr || executed[pass]) { return; }
            executed[pass] = true;

            for (auto& dep : deps) {
                execute(ctx, graph, dep);
            }

            for (auto& input : pass->inputs) {
                auto& wire = graph.wires.at(input.get());
                input->setResource(wire->getResource());
            }

            pass->execute(ctx);
            graph.channel.info("executed pass: {}", pass->getName());
        }

    private:
        std::unordered_map<Output*, Resource*> resources;
        std::unordered_map<Pass*, bool> executed;
    };
}

void Graph::init(Context& ctx) {
    for (auto& [name, pass] : passes) {
        pass->init(ctx);
        channel.info("initialized pass: {}", name);
    }
}

void Graph::deinit(Context& ctx) {
    for (auto& [name, pass] : passes) {
        pass->deinit(ctx);
        channel.info("deinitialized pass: {}", name);
    }
}

void Graph::execute(Context& ctx, Pass *root) {
    GraphBuilder builder;
    auto tree = builder.build(*this, root);
    builder.execute(ctx, *this, tree);
}
