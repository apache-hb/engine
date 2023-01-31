#pragma once

#include "simcoe/render/context.h"

#include <unordered_map>

namespace simcoe::render {
    struct Graph;

    struct GraphObject {
        GraphObject(const char *pzName, Graph& graph);
        virtual ~GraphObject() = default;

        const char *getName() const;
        Graph& getGraph();
        Context& getContext();

    private:
        const char *pzName;
        struct Graph& graph;
    };

    struct Pass : GraphObject {
        using GraphObject::GraphObject;

        virtual void start() = 0;
        virtual void stop() = 0;
        virtual void execute() = 0;
    };

    struct Resource : GraphObject {
        virtual void create() = 0;
        virtual void destroy() = 0;
    };

    struct Wire : GraphObject {
        
    };

    struct Graph {
        Graph(Context& context);

        void start();
        void stop();

        Context& getContext();

    protected:
        void execute(Pass *pRoot);

        template<typename T>
        T *addPass(const char *pzName) {
            T *pPass = new T(pzName, *this);
            passes[pzName] = pPass;
            return pPass;
        }

    private:
        Context& context;

        std::unordered_map<const char*, Pass*> passes;
    };
}
