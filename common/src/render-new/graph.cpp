#include "engine/render-new/graph.h"
#include "engine/base/logging.h"
#include "engine/render-new/render.h"
#include "engine/rhi/rhi.h"

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

        void wireBarriers(Context& ctx, Graph& graph, Pass *pass) {
            std::vector<rhi::StateTransition> barriers;

            for (auto& input : pass->inputs) {
                auto& output = graph.wires.at(input.get());
                output->update(barriers, input.get());

                ASSERTF(output->resource != nullptr, "output `{}` from pass `{}` was null", output->getName(), pass->getName());
            }

            if (barriers.empty()) { return; }

            ctx.transition(CommandSlot::ePost, barriers);
        }

        void execute(Context& ctx, Graph& graph, Tree& tree) {
            auto& [pass, deps] = tree;
            
            if (pass == nullptr || executed[pass]) { return; }
            executed[pass] = true;

            for (auto& dep : deps) {
                execute(ctx, graph, dep);
            }

            logging::get(logging::eRender).info("transitioning into pass `{}`", pass->getName());

            wireBarriers(ctx, graph, pass);

            pass->execute(ctx);
        }

    private:
        std::unordered_map<Output*, Resource*> resources;
        std::unordered_map<Pass*, bool> executed;
    };
}

void Resource::addBarrier(Barriers &, Output *before, Input *after) {
    ASSERT(after != nullptr && before != nullptr);
    after->resource = before->resource;
}

Context& render::getContext(const Info& info) {
    return info.parent->getContext();
}

Graph *Resource::getParent() const { return info.parent; }
const char *Resource::getName() const { return info.name; }
Context& Resource::getContext() const { return getParent()->getContext(); }

Graph *Pass::getParent() const { return info.parent; }
const char *Pass::getName() const { return info.name; }
Context& Pass::getContext() const { return getParent()->getContext(); }

void Output::update(Barriers& barriers, Input* input) { 
    ASSERTF(resource != nullptr, "output `{}:{}` was null", getName(), getPass()->getName());
    resource->addBarrier(barriers, this, input);
}

void Relay::update(Barriers &barriers, Input *other) {
    resource = input->resource;
    Output::update(barriers, other);
}

void Graph::init() {
    for (auto& [name, pass] : passes) {
        pass->init(ctx);
        channel.info("initialized pass: {}", name);
    }
    
    ctx.getCopyQueue().wait();
}

void Graph::deinit() {
    for (auto& [name, pass] : passes) {
        pass->deinit(ctx);
        channel.info("deinitialized pass: {}", name);
    }
}

void Graph::execute(Pass *root) {
    GraphBuilder builder;
    auto tree = builder.build(*this, root);
    
    builder.execute(ctx, *this, tree);
    channel.info("finished frame");
}
