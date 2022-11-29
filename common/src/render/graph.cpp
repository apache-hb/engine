#include "engine/render/graph.h"
#include "engine/base/logging.h"
#include "engine/render/render.h"
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

            ctx.transition(pass->getSlot(), barriers);
        }

        void execute(Context& ctx, Graph& graph, Tree& tree) {
            auto& [pass, deps] = tree;
            
            if (pass == nullptr || executed[pass]) { return; }
            executed[pass] = true;

            auto slot = pass->getSlot();
            if (!slots[slot]) {
                ctx.beginFrame(slot);
                slots[slot] = true;
            }

            for (auto& dep : deps) {
                execute(ctx, graph, dep);
            }

            wireBarriers(ctx, graph, pass);
            
            pass->execute();
        }

    private:
        std::unordered_map<Output*, Resource*> resources;
        std::unordered_map<Pass*, bool> executed;
        bool slots[CommandSlot::eTotal] = { false };
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
rhi::CommandList& Pass::getCommands() { return getContext().getCommands(getSlot()); }

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
        pass->init();
        channel.info("initialized pass: {}", name);
    }
    
    ctx.getCopyQueue().wait();
}

void Graph::deinit() {
    for (auto& [name, pass] : passes) {
        pass->deinit();
        channel.info("deinitialized pass: {}", name);
    }
}

void Graph::execute(Pass *root) {
    GraphBuilder builder;
    auto tree = builder.build(*this, root);
    
    builder.execute(ctx, *this, tree);
}
