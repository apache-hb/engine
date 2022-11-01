#include "engine/render-new/render.h"
#include "engine/rhi/rhi.h"

using namespace simcoe;
using namespace simcoe::render;

namespace {
    constexpr size_t getMaxHeapSize(const ContextInfo& info) {
        return info.maxObjects      // object buffers
             + info.maxTextures     // texture buffers
             + 1                    // dear imgui
             + 1;                   // intermediate buffer
    }
}

Context::Context(const ContextInfo& info)
    : info(info)
    , device(rhi::getDevice())
    , presentQueue(device, info)
    , copyQueue(device, info)
    , cbvHeap(device, getMaxHeapSize(info), rhi::DescriptorSet::Type::eConstBuffer, true)
{ 
    updateViewport(sceneSize());
}

void Context::updateViewport(rhi::TextureSize scene) {
    auto [width, height] = windowSize();;

    auto widthRatio = float(scene.width) / width;
    auto heightRatio = float(scene.height) / height;

    float x = 1.f;
    float y = 1.f;

    if (widthRatio < heightRatio) {
        x = widthRatio / heightRatio;
    } else {
        y = heightRatio / widthRatio;
    }

    viewport.viewport.TopLeftX = width * (1.f - x) / 2.f;
    viewport.viewport.TopLeftY = height * (1.f - y) / 2.f;
    viewport.viewport.Width = x * width;
    viewport.viewport.Height = y * height;

    viewport.scissor.left = LONG(viewport.viewport.TopLeftX);
    viewport.scissor.right = LONG(viewport.viewport.TopLeftX + viewport.viewport.Width);
    viewport.scissor.top = LONG(viewport.viewport.TopLeftY);
    viewport.scissor.bottom = LONG(viewport.viewport.TopLeftY + viewport.viewport.Height);
}
