#pragma once

#include "system/system.h"
#include "util.h"

namespace engine::render {
    struct Context {
        Context() = default;
        Context(Factory factory, size_t adapterIndex, system::Window *window, UINT frameCount)
            : factory(factory)
            , adapterIndex(adapterIndex)
            , window(window)
            , frameCount(frameCount) 
        { }

        /// data given in from the constructor
        Factory factory;
        size_t adapterIndex;
        system::Window *window;
        UINT frameCount;
    };

    Context createContext(Factory factory, system::Window *window, size_t adapter, UINT frameCount);
}
