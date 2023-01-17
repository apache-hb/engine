#pragma once

#include <map>

namespace simcoe::render::graph {
    struct Graph;
    struct Pass;
    struct Resource;
    struct Handle;

    struct GraphObject {
        constexpr GraphObject(Graph *pGraph, const char *pzName)
            : pGraph(pGraph)
            , pzName(pzName)
        { }

        constexpr Graph *getGraph() const;
        constexpr const char *getName() const;

    private:
        Graph *pGraph;
        const char *pzName;
    };

    struct Graph {
        void execute();

    protected:
        constexpr Graph(Pass *pStart)
            : pStart(pStart)
        { }

    private:
        Pass *pStart;
    };

    struct Pass : GraphObject {
        virtual void init() = 0;
        virtual void deinit() = 0;
        virtual void execute() = 0;
        virtual void dbg() = 0;
    };

    struct Resource : GraphObject {
        
    };

    struct Handle : GraphObject {
        
    };
}
