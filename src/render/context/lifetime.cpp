#include "render/context.h"
#include "render/objects/factory.h"

using namespace engine::render;

void Context::createDevice() {
    device = getAdapter().newDevice(D3D_FEATURE_LEVEL_11_0) % "context-device";
}

void Context::destroyDevice() {
    device.tryDrop("context-device");
}

void Context::createViews() {
// get our current internal and external resolution
    auto [displayWidth, displayHeight] = getPostResolution();
    auto [internalWidth, internalHeight] = getSceneResolution();

    // calculate required scaling to prevent displaying
    // our drawn image outside the display area
    auto widthRatio = float(internalWidth) / float(displayWidth);
    auto heightRatio = float(internalHeight) / float(displayHeight);

    float x = 1.f;
    float y = 1.f;

    if (widthRatio < heightRatio) { 
        x = widthRatio / heightRatio; 
    } else { 
        y = heightRatio / widthRatio; 
    }

    auto left = float(displayWidth) * (1.f - x) / 2.f;
    auto top = float(displayHeight) * (1.f - y) / 2.f;
    auto width = float(displayWidth * x);
    auto height = float(displayHeight * y);

    // set our viewport and scissor rects
    postView = View(top, left, width, height);
    sceneView = View(0.f, 0.f, float(internalWidth), float(internalHeight));

    displayResolution = Resolution(displayWidth, displayHeight);
    sceneResolution = Resolution(internalWidth, internalHeight);
}
