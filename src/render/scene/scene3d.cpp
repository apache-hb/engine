#include "scene.h"

#include "render/render.h"
#include "render/objects/commands.h"

using namespace engine::render;

void Scene3D::create() {
    createCommandList();
}

void Scene3D::destroy() {
    destroyCommandList();
}

ID3D12CommandList* Scene3D::populate() {
    auto& allocator = ctx->getAllocator(Allocator::Scene);
    const auto [scissor, viewport] = ctx->getSceneView();
    auto targetHandle = ctx->sceneTargetRtvCpuHandle();

    check(allocator->Reset(), "failed to reset allocator");
    check(commandList->Reset(allocator.get(), nullptr), "failed to reset command list");

    commandList->RSSetScissorRects(1, &scissor);
    commandList->RSSetViewports(1, &viewport);

    commandList->OMSetRenderTargets(1, &targetHandle, false, nullptr);
    commandList->ClearRenderTargetView(targetHandle, kClearColour, 0, nullptr);

    check(commandList->Close(), "failed to close command list");
    return commandList.get();
}

void Scene3D::createCommandList() {
    auto& device = ctx->getDevice();
    auto& allocator = ctx->getAllocator(Allocator::Scene);
    commandList = device.newCommandList(L"scene-command-list", commands::kDirect, allocator);
    check(commandList->Close(), "failed to close command list");
}

void Scene3D::destroyCommandList() {
    commandList.tryDrop("scene-command-list");
}
