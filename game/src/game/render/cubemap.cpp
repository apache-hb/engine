#pragma once

#include "game/render.h"

using namespace game;

CubeMapPass::CubeMapPass(const GraphObject& object) : render::Pass(object) { 
    pRenderTargetIn = in<render::InEdge>("render-target");
    pRenderTargetOut = out<render::RelayEdge>("render-target", pRenderTargetIn);
}

void CubeMapPass::start() {

}

void CubeMapPass::stop() {

}

void CubeMapPass::execute() {

}
