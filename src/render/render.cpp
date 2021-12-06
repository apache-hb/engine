#include "render.h"

namespace engine::render {
    Context createContext(Factory factory, system::Window *window, size_t adapter, UINT frameCount) {
        return Context(factory, adapter, window, frameCount);
    }
}
