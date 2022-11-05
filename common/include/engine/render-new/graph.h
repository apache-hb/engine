#pragma once

#include "engine/base/logging.h"
#include "engine/render-new/render.h"

#include <unordered_map>

namespace simcoe::render {
    struct Resource;
    struct Pass;
    struct Graph;
    struct Input;
    struct Output;

    using State = rhi::Buffer::State;
    using Barriers = std::vector<rhi::StateTransition>;

    struct Info {
        Graph *parent;
        const char *name;
    };

    Context& getContext(const Info& info);

    struct Resource {
        Resource(const Info& info)
            : info(info)
        { }

        virtual ~Resource() = default;

        virtual void addBarrier(Barriers& barriers, Output* before, Input* after);
    
        Graph *getParent() const;
        const char *getName() const;
        Context& getContext() const;

    private:
        Info info;
    };

    struct Wire {
        virtual ~Wire() = default;

        const char *getName() const { return name; }
        Pass *getPass() const { return pass; }
        State getState() const { return state; }

        Resource *resource;

    protected:
        Wire(Pass *pass, const char *name, State state, Resource *resource)
            : resource(resource)
            , name(name)
            , pass(pass)
            , state(state)
        { }

    private:
        const char *name;
        Pass *pass;
        rhi::Buffer::State state;
    };

    struct Input : Wire {
        using Wire::Wire;
        Input(Pass *pass, const char *name, State state = State::eInvalid)
            : Wire(pass, name, state, nullptr)
        { }
    };

    struct Output : Wire {
        using Wire::Wire;
        Output(Pass *pass, const char *name, State state = State::eInvalid)
            : Wire(pass, name, state, nullptr)
        { }
        
        virtual void update(Barriers& barriers, Input* input);
    };

    struct Source : Output {
        Source(Pass *pass, const char *name, State initial, Resource *resource) : Output(pass, name, initial, resource) { 
            ASSERTF(resource != nullptr, "source `{}` was null", name);
        }
    };

    struct Relay : Output {
        Relay(Pass *pass, const char *name, Input *input) 
            : Output(pass, name, input->getState(), input->resource)
            , input(input)
        { }

        void update(Barriers& barriers, Input* other) override;

    private:
        Input *input;
    };

    template<typename TWire, typename TResource>
    struct WireHandle {
        WireHandle(TWire *wire = nullptr) : wire(wire) { }

        TResource *get() { return static_cast<TResource*>(wire->resource); }

        operator TWire *() { return wire; }

    private:
        TWire *wire;
    };

    struct Pass {
        Pass(const Info& info)
            : info(info)
        { }

        virtual ~Pass() = default;

        virtual void init(Context&) { }
        virtual void deinit(Context&) { }

        virtual void execute(Context& ctx) = 0;

        template<typename T, typename... A>  requires (std::is_base_of_v<Input, T>)
        T *newInput(const char *name, A&&... args) {
            auto input = new T(this, name, std::forward<A>(args)...);
            inputs.push_back(input);
            return input;
        }

        template<typename T, typename... A> requires (std::is_base_of_v<Output, T>)
        T *newOutput(const char *name, A&&... args) {
            auto output = new T(this, name, std::forward<A>(args)...);
            outputs.push_back(output);
            return output;
        }

        std::vector<UniquePtr<Input>> inputs;
        std::vector<UniquePtr<Output>> outputs;

        const char *getName() const;
        Graph *getParent() const;
        Context& getContext() const;

    private:
        Info info;
    };

    // render graph
    struct Graph {
        Graph(Context& ctx)
            : ctx(ctx)
        { }

        void init();
        void deinit();

        void execute(Pass *root);

        template<typename T, typename... A> requires (std::is_base_of_v<Pass, T>)
        T *addPass(const char *name, A&&... args) {
            auto pass = new T({ this, name }, std::forward<A>(args)...);
            passes[name] = pass;
            return pass;
        }

        template<typename T, typename... A> requires (std::is_base_of_v<Resource, T>)
        T *addResource(const char *name, A&&... args) {
            auto resource = new T({ this, name }, std::forward<A>(args)...);
            resources[name] = resource;
            return resource;
        }

        void link(Input *input, Output *output) {
            wires[input] = output;
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

        Context& getContext() const { return ctx; }

    protected:
        Context& ctx;
    };
}
