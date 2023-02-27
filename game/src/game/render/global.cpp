#pragma once

#include "game/render.h"

using namespace game;

GlobalPass::GlobalPass(const GraphObject& object, Info& info) 
    : Pass(object, info) 
{ 
    pRenderTargetOut = out<RenderEdge>("render-target");
}

void GlobalPass::start(ID3D12GraphicsCommandList*) {
    pRenderTargetOut->start();
}

void GlobalPass::stop() {
    pRenderTargetOut->stop();
}

void GlobalPass::execute(ID3D12GraphicsCommandList* pCommands) {
    ID3D12DescriptorHeap *ppHeaps[] = { getContext().getCbvHeap().getHeap() };
    pCommands->SetDescriptorHeaps(UINT(std::size(ppHeaps)), ppHeaps);
}
