#include "render/render.h"

namespace engine::render {
    Context::Context(Context::Create&& create): info(create) {
        buildViews();
        createDevice();
        createBuffers();
    }

    Context::~Context() {
        destroyBuffers(); 
        destroyDevice();
    }
}
