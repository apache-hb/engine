#pragma once

#include "simcoe/render/context.h"

namespace simcoe::render {
    struct GraphObject {
        const char *pzName;
        struct Graph *pGraph;
    };

    struct Pass : GraphObject {
        virtual void start() = 0;
        virtual void stop() = 0;
        virtual void execute() = 0;
    };

    struct Resource : GraphObject {
        virtual void create() = 0;
        virtual void destroy() = 0;
    };

    struct Wire : GraphObject {
        D3D12_RESOURCE_STATES expected;
    };

    struct Graph {
        Graph(Context& context);
        ~Graph();

        void execute();

    private:
        Context& context;
    };
}
