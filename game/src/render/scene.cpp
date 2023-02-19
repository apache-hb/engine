#include "game/render.h"

using namespace game;
using namespace simcoe;

ScenePass::ScenePass(const GraphObject& object, const system::Size& size) 
    : Pass(object)
    , size(size)
{
    pRenderTargetOut = out<IntermediateTargetEdge>("scene-target", size);
}

void ScenePass::start() {
    pRenderTargetOut->start();
}

void ScenePass::stop() {
    pRenderTargetOut->stop();
}

void ScenePass::execute() {
    auto cmd = getContext().getCommandList();
    auto rtv = pRenderTargetOut->cpuHandle(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    Display display = createDisplay(size);

    cmd->RSSetViewports(1, &display.viewport);
    cmd->RSSetScissorRects(1, &display.scissor);

    cmd->OMSetRenderTargets(1, &rtv, false, nullptr);
    cmd->ClearRenderTargetView(rtv, kClearColour, 0, nullptr);
}

