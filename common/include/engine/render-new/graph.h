#pragma once

#include "engine/base/logging.h"
#include "engine/render-new/render.h"
#include "engine/rhi/rhi.h"

#include <unordered_map>

namespace simcoe::render {
    struct Resource;
    struct Pass;
    struct Graph;
    struct Input;
    struct Output;

    struct Resource {
        virtual ~Resource() = default;
    };

    struct Wire {
        const char *getName() const { return name; }
        Pass *getPass() const { return pass; }
        Resource *getResource() const { return resource; }

        virtual void setResource(Resource *it) { this->resource = it; }

    protected:
        Wire(Pass *pass, const char *name, Resource *resource)
            : name(name)
            , pass(pass)
            , resource(resource) 
        { }

    private:
        const char *name;
        Pass *pass;
        Resource *resource;
    };

    struct Input : Wire {
        using Wire::Wire;
        Input(Pass *pass, const char *name)
            : Wire(pass, name, nullptr)
        { }

        virtual ~Input() = default;
    };

    struct InputList : Input {
        using Input::Input;

        void setResource(Resource *it) override {
            resources.push_back(it);
        }

    private:
        std::vector<Resource*> resources;
    };

    struct Output : Wire {
        using Wire::Wire;
        Output(Pass *pass, const char *name, Resource *resource = nullptr)
            : Wire(pass, name, resource)
        { }
    };

    struct Pass {
        Pass(const char *name)
            : name(name)
        { }

        virtual ~Pass() = default;

        virtual void init(Context&) { }
        virtual void deinit(Context&) { }

        virtual void execute(Context& ctx) = 0;

        template<typename T, typename... A>
        T *newInput(const char *name, A&&... args) {
            auto input = new T(this, name, std::forward<A>(args)...);
            inputs.push_back(input);
            return input;
        }

        template<typename T, typename... A>
        T *newOutput(const char *name, A&&... args) {
            auto output = new T(this, name, std::forward<A>(args)...);
            outputs.push_back(output);
            return output;
        }

        std::vector<UniquePtr<Input>> inputs;
        std::vector<UniquePtr<Output>> outputs;

        const char *getName() const { return name; }
    private:
        const char *name;
    };

    // render graph
    struct Graph {
        void init(Context& ctx);
        void deinit(Context& ctx);

        void execute(Context& ctx, Pass *root);

        template<typename T, typename... A> requires (std::is_base_of_v<Pass, T>)
        T *addPass(const char *name, A&&... args) {
            auto pass = new T(name, std::forward<A>(args)...);
            passes[name] = pass;
            return pass;
        }

        template<typename T, typename... A> requires (std::is_base_of_v<Resource, T>)
        T *addResource(const char *name, A&&... args) {
            auto resource = new T(name, std::forward<A>(args)...);
            resources[name] = resource;
            return resource;
        }

        void link(Input *input, Output *output) {
            wires[input] = output;
        }

        void link(Pass *dep, Pass *pass) {
            auto input = pass->newInput<Input>("control");
            auto output = dep->newOutput<Output>("control");
            link(input, output);
        }

        template<typename T, typename... A> requires (std::is_base_of_v<Output, T>)
        UniquePtr<T> provide(const char *name, Resource *resource) {
            return new T(nullptr, name, resource);
        }

        // TODO: this probably shouldnt be public
        std::unordered_map<const char*, UniquePtr<Pass>> passes;
        std::unordered_map<const char*, UniquePtr<Resource>> resources;
        std::unordered_map<Input*, Output*> wires; // map of input to the output it is reading from
    
        logging::Channel& channel = logging::get(logging::eRender);
    };
}
