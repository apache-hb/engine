#include "game/render.h"

using namespace game;

CubeMapPass::CubeMapPass(const GraphObject& object, Info& info) 
    : Pass(object, info) 
{ 
    pRenderTargetIn = in<render::InEdge>("render-target", D3D12_RESOURCE_STATE_RENDER_TARGET);
    pRenderTargetOut = out<render::RelayEdge>("render-target", pRenderTargetIn);
}

void CubeMapPass::start(ID3D12GraphicsCommandList*) {

}

void CubeMapPass::stop() {

}

void CubeMapPass::execute(ID3D12GraphicsCommandList*) {
    
}
