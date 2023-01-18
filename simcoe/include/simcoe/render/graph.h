#pragma once

#include "simcoe/render/context.h"

namespace simcoe::render {
    struct Graph {
        Graph(Context& context);
        ~Graph();

        void execute();

    private:
        Context& context;
    };
}
