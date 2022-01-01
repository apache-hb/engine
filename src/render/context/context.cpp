#include "render/render.h"

using namespace engine::render;

Context::Context(Create&& create): info(create) {
    createDevice(); // create our render context
    createViews(); // calculate our viewport and scissor rects
    createDirectCommandQueue(); // create our direct command queue to present with
    createUploadCommandQueue(); // create our upload command queue to copy with
    createSwapChain(); // create our swap chain to present to an hwnd with

    // create our rtv heap to hold our swap chain buffers
    // as well as our intermediate scene target
    createRtvHeap();

    // create our constant buffers to store most shader visible data
    // this includes our intermediate scene target
    createCbvHeap();

    // create our intermediate render target to render our scenes to
    // this allows us to do resolution scaling and post processing
    createSceneTarget();

    // create our per-backbuffer data
    createFrameData();

    // create our fence for synchronization
    createFence();
}

Context::~Context() {
    destroyFence();
    destroyFrameData();
    destroySceneTarget();
    destroyRtvHeap();
    destroyCbvHeap();
    destroySwapChain();
    destroyUploadCommandQueue();
    destroyDirectCommandQueue();
    destroyDevice();
}

bool Context::present() {
    return true;
}
