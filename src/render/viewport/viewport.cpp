#pragma once

#include "viewport.h"

#include "render/objects/commands.h"
#include "render/objects/signature.h"
#include "render/render.h"

#include <array>
#include <DirectXMath.h>

using namespace engine::render;

struct ScreenVertex {
    DirectX::XMFLOAT4 position;
    DirectX::XMFLOAT2 texcoord;
};

constexpr auto kScreenQuad = std::to_array<ScreenVertex>({
    { { -1.f, -1.f, 0.f, 1.f }, { 0.f, 1.f } },
    { { -1.f,  1.f, 0.f, 1.f }, { 0.f, 0.f } },
    { {  1.f, -1.f, 0.f, 1.f }, { 1.f, 1.f } },
    { {  1.f,  1.f, 0.f, 1.f }, { 1.f, 0.f } }
});

constexpr auto kShaderLayout = std::to_array({
    shaderInput("POSITION", DXGI_FORMAT_R32G32B32A32_FLOAT),
    shaderInput("TEXCOORD", DXGI_FORMAT_R32G32_FLOAT)
});

constexpr ShaderLibrary::Create kShaderInfo = {
    .path = L"resources\\shaders\\post-shader.hlsl",
    .vsMain = "vsMain",
    .psMain = "psMain",
    .layout = { kShaderLayout }
};

constexpr auto kRanges = std::to_array({
    srvRange(0, 1)
});

constexpr auto kParams = std::to_array({
    tableParameter(D3D12_SHADER_VISIBILITY_PIXEL, kRanges)
});

constexpr D3D12_STATIC_SAMPLER_DESC kSamplers[] = {{
    .Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR,
    .AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER,
    .AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER,
    .AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER,
    .ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER,
    .BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK,
    .MinLOD = 0.0f,
    .MaxLOD = D3D12_FLOAT32_MAX,
    .ShaderRegister = 0,
    .RegisterSpace = 0,
    .ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL
}};

constexpr D3D12_ROOT_SIGNATURE_FLAGS kFlags =
    D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
    D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS       |
    D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS     |
    D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

constexpr RootCreate kRootInfo = {
    .params = kParams,
    .samplers = kSamplers,
    .flags = kFlags
};

DisplayViewport::DisplayViewport(Context* context): ctx(context) {
    shaders = ShaderLibrary(kShaderInfo);
}

void DisplayViewport::create() {
    createScreenQuad();
}

void DisplayViewport::destroy() {
    destroyScreenQuad();
}

ID3D12CommandList* DisplayViewport::populate() {
    return commandList.get();
}

void DisplayViewport::createScreenQuad() {
    screenQuad = ctx->uploadData(L"screen-quad", kScreenQuad.data(), kScreenQuad.size() * sizeof(ScreenVertex));
}

void DisplayViewport::destroyScreenQuad() {
    screenQuad.tryDrop("screen-quad");
}



void DisplayViewport::createCommandList() {
    auto& allocator = ctx->getAllocator(Allocator::Target);
    commandList = getDevice().newCommandList(L"viewport-command-list", commands::kDirect, allocator);
}

void DisplayViewport::destroyCommandList() {
    commandList.tryDrop("viewport-command-list");
}



void DisplayViewport::createRootSignature() {

}

void DisplayViewport::destroyRootSignature() {
    rootSignature.tryDrop("viewport-root-signature");
}



void DisplayViewport::createPipelineState() {

}

void DisplayViewport::destroyPipelineState() {
    pipelineState.tryDrop("viewport-pipeline-state");
}

Device<ID3D12Device4>& DisplayViewport::getDevice() {
    return ctx->getDevice();
}
