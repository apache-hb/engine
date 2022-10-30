#pragma once

#include "engine/graph/pass.h"
#include "engine/graph/resource.h"

namespace simcoe::graph {
    struct RenderGraph;
    struct ResourceInput;

    struct RenderPassSlot {
        RenderPass *pass;
        const char *slot;
    };

    struct SlotBinding {
        RenderPassSlot source;
        RenderPassSlot target;
    };

    struct RenderGraph {
        RenderGraph(const Create& create);

        template<typename T, typename... A> requires (std::is_base_of_v<RenderPass, T>)
        T *newPass(const char* name, A&&... args) {
            ASSERTF(passes.find(name) == passes.end(), "Render pass with name '{}' already exists", name);

            Info init = { this, name };
            auto pass = new T(init, std::forward<A>(args)...);
            passes[name] = pass;
            return pass;
        }

        template<typename T, typename... A> requires (std::is_base_of_v<RenderResource, T>)
        T *newResource(const char* name, A&&... args) {
            ASSERTF(resources.find(name) == resources.end(), "Render resource with name '{}' already exists", name);

            Info init = { this, name };
            auto resource = new T(init, std::forward<A>(args)...);
            resources[name] = resource;
            return resource;
        }

        void render();

        void bind(RenderPassSlot source, RenderPassSlot target);

        const Create& getInfo() const { return info; }

    private:
        Create info;

        // render graph passes
        std::unordered_map<const char*, UniquePtr<RenderPass>> passes;
        std::unordered_map<const char*, UniquePtr<RenderResource>> resources;

        std::vector<SlotBinding> bindings;
    };
}
