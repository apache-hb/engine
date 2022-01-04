#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_win32.h"
#include "imgui/backends/imgui_impl_dx12.h"

#include "viewport.h"

#include "render/objects/factory.h"
#include "render/objects/commands.h"
#include "render/objects/signature.h"
#include "render/render.h"

#include <array>
#include <DirectXMath.h>

using namespace DirectX;
using namespace engine::render;

constexpr float kBorderColour[] = { 0.0f, 0.0f, 0.0f, 1.0f };

struct ScreenVertex {
    XMFLOAT4 position;
    XMFLOAT2 texcoord;
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
    createRootSignature();
    createPipelineState();
    createCommandList();

    auto heap = ctx->getCbvHeap();

    ImGui_ImplDX12_Init(getDevice().get(), ctx->getBufferCount(),
        ctx->getFormat(), heap.get(),
        heap.cpuHandle(Resources::ImGui),
        heap.gpuHandle(Resources::ImGui)
    );
}

void DisplayViewport::destroy() {
    ImGui_ImplDX12_Shutdown();

    destroyCommandList();
    destroyPipelineState();
    destroyRootSignature();
    destroyScreenQuad();
}

ID3D12CommandList* DisplayViewport::populate() {
    const auto [internalWidth, internalHeight] = ctx->sceneResolution;
    const auto [displayWidth, displayHeight] = ctx->displayResolution;

    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    auto info = ctx->getAdapter().getMemoryInfo();

    ImGui::Begin("Render Info");
        ImGui::Text("budget: %s", info.budget.string().c_str());
        ImGui::Text("used: %s", info.used.string().c_str());
        ImGui::Text("available: %s", info.available.string().c_str());
        ImGui::Text("committed: %s", info.committed.string().c_str());
        ImGui::Separator();
        ImGui::Text("internal resolution: %ux%u", internalWidth, internalHeight);
        ImGui::Text("display resolution: %ux%u", displayWidth, displayHeight);
    ImGui::End();

    ImGui::ShowDemoWindow();

    ImGui::Render();

    const auto frame = ctx->getCurrentFrame();
    auto drawData = ImGui::GetDrawData();
    const auto [scissor, viewport] = ctx->getPostView();

    auto& allocator = ctx->getAllocator(Allocator::Viewport);
    auto target = ctx->getTarget();
    auto sceneTarget = ctx->getSceneTarget();
    auto targetHandle = ctx->rtvHeapCpuHandle(frame);
    auto heap = ctx->getCbvHeap();
    ID3D12DescriptorHeap* heaps[] = { heap.get() };

    D3D12_RESOURCE_BARRIER inTransitions[] = {
        CD3DX12_RESOURCE_BARRIER::Transition(target.get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET),
        CD3DX12_RESOURCE_BARRIER::Transition(sceneTarget.get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE)
    };

    D3D12_RESOURCE_BARRIER outTransitions[] = {
        CD3DX12_RESOURCE_BARRIER::Transition(target.get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT),
        CD3DX12_RESOURCE_BARRIER::Transition(sceneTarget.get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET)
    };

    check(allocator->Reset(), "failed to reset viewport allocator");
    check(commandList->Reset(allocator.get(), pipelineState.get()), "failed to reset viewport command list");

    commandList->ResourceBarrier(UINT(std::size(inTransitions)), inTransitions);

    commandList->RSSetScissorRects(1, &scissor);
    commandList->RSSetViewports(1, &viewport);

    commandList->OMSetRenderTargets(1, &targetHandle, false, nullptr);
    commandList->ClearRenderTargetView(targetHandle, kBorderColour, 0, nullptr);

    commandList->SetGraphicsRootSignature(rootSignature.get());
    commandList->SetDescriptorHeaps(UINT(std::size(heaps)), heaps);
    commandList->SetGraphicsRootDescriptorTable(0, heap.gpuHandle(Resources::SceneTarget));

    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    commandList->IASetVertexBuffers(0, 1, &screenQuad.view);
    commandList->DrawInstanced(4, 1, 0, 0);

    ImGui_ImplDX12_RenderDrawData(drawData, commandList.get());

    commandList->ResourceBarrier(UINT(std::size(outTransitions)), outTransitions);

    check(commandList->Close(), "failed to close viewport command list");
    return commandList.get();
}

void DisplayViewport::createScreenQuad() {
    screenQuad = ctx->uploadVertexBuffer<ScreenVertex>(L"screen-quad", kScreenQuad);
}

void DisplayViewport::destroyScreenQuad() {
    screenQuad.tryDrop("screen-quad");
}



void DisplayViewport::createCommandList() {
    auto& allocator = ctx->getAllocator(Allocator::Viewport);
    commandList = getDevice().newCommandList(L"viewport-command-list", commands::kDirect, allocator);
    check(commandList->Close(), "failed to close viewport command list");
}

void DisplayViewport::destroyCommandList() {
    commandList.tryDrop("viewport-command-list");
}



void DisplayViewport::createRootSignature() {
    rootSignature = getDevice().newRootSignature(L"viewport-root-signature", kRootInfo);
}

void DisplayViewport::destroyRootSignature() {
    rootSignature.tryDrop("viewport-root-signature");
}



void DisplayViewport::createPipelineState() {
    pipelineState = getDevice().newGraphicsPSO(L"viewport-pipeline-state", rootSignature, {
        .shaders = shaders,
        .dsvFormat = DXGI_FORMAT_UNKNOWN
    });
}

void DisplayViewport::destroyPipelineState() {
    pipelineState.tryDrop("viewport-pipeline-state");
}

Device<ID3D12Device4>& DisplayViewport::getDevice() {
    return ctx->getDevice();
}
