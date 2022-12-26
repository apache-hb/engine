#include "engine/render/passes/present.h"

using namespace simcoe;
using namespace simcoe::render;

PresentPass::PresentPass(const Info& info)
    : Pass(info, CommandSlot::ePost)
{
    renderTargetIn = newInput<Input>("rtv", State::ePresent);
    sceneTargetIn = newInput<Input>("scene-target", State::eRenderTarget);
}

void PresentPass::execute() {
    auto& ctx = getContext();
    ctx.endFrame();
    ctx.present();
}
