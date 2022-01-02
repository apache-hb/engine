#pragma once

namespace engine::render {
    struct Context;

    struct Scene {
        Scene(Context* context)
            : context(context)
        { }

        void create();
        void destroy();

        void populate();

    protected:
        Context* context;
    };
}
