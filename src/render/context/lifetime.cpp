#include "render/context.h"
#include "render/objects/factory.h"

using namespace engine::render;

constexpr auto kSwapSampleCount = 1;
constexpr auto kSwapSampleQuality = 0;

constexpr auto kDefaultProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

void Context::createDevice() {
    device = getAdapter().newDevice(D3D_FEATURE_LEVEL_11_0) % "device";
    device.attachInfoQueue([](auto category, auto severity, auto id, auto desc, UNUSED void* ctx) {
        auto cat = int(category);
        auto i = int(id);
        switch (severity) {
        case D3D12_MESSAGE_SEVERITY_INFO:
        case D3D12_MESSAGE_SEVERITY_MESSAGE:
            log::render->info("[{}.{}] {}", cat, i, desc);
            break;
        case D3D12_MESSAGE_SEVERITY_WARNING:
            log::render->warn("[{}.{}] {}", cat, i, desc);
            break;
        default:
            log::render->fatal("[{}.{}] {}", cat, i, desc);
            break;
        }
    });
}

void Context::destroyDevice() {
    device.tryDrop("device");
}

void Context::createCommandQueues() {
    for (size_t i = 0; i < Queues::Total; i++) {
        auto index = Queues::Index(i);
        queues[i] = device.newCommandQueue(Queues::type(index)) % Queues::name(index);
    }
}

void Context::destroyCommandQueues() {
    for (size_t i = 0; i < Queues::Total; i++) {
        queues[i].tryDrop(Queues::name(Queues::Index(i)));
    }
}

void Context::createSwapChain() {
    auto& factory = getFactory();
    const auto [width, height] = getPostResolution();
    const auto format = getFormat();
    const auto bufferCount = getBufferCount();
    const auto flags = factory.flags();

    const DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {
        .Width = width,
        .Height = height,
        .Format = format,
        .SampleDesc = { kSwapSampleCount, kSwapSampleQuality },
        .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
        .BufferCount = bufferCount,
        .SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
        .Flags = flags
    };

    swapchain = factory.newSwapChain(queues[Queues::Direct], getWindow().getHandle(), swapChainDesc);
    frameIndex = swapchain->GetCurrentBackBufferIndex();
}

void Context::destroySwapChain() {
    swapchain.tryDrop("swapchain");
}

void Context::createRtvHeap() {
    rtvHeap = device.newHeap(DescriptorHeap::kRtv, Targets::Total + getBufferCount()) % "rtv-heap";
}

void Context::destroyRtvHeap() {
    rtvHeap.tryDrop("rtv-heap");
}

void Context::createCbvHeap() {
    cbvHeap = device.newHeap(DescriptorHeap::kCbvSrvUav, Targets::Total) % "cbv-heap";
}

void Context::destroyCbvHeap() {
    cbvHeap.tryDrop("cbv-heap");
}

void Context::createFrameData() {
    const auto format = getFormat();
    const auto [width, height] = getPostResolution();
    const auto clear = CD3DX12_CLEAR_VALUE(format, kClearColour);
    const auto desc = CD3DX12_RESOURCE_DESC::Tex2D(
        /* format = */ format,
        /* width = */ width,
        /* height = */ height,
        /* arraySize = */ 1,
        /* mipLevels = */ 1,
        /* sampleCount = */ kSwapSampleCount,
        /* sampleQuality = */ kSwapSampleQuality,
        /* flags = */ D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET
    );
    const auto count = getBufferCount();

    sceneTarget = device.newCommittedResource(kDefaultProps, desc, D3D12_RESOURCE_STATE_RENDER_TARGET, &clear) % "scene-target";

    bindRtv(sceneTarget, rtvHeap.cpuHandle(Targets::Scene));
    bindSrv(sceneTarget, cbvHeap.cpuHandle(Targets::Scene));

    for (UINT i = 0; i < count; i++) {
        auto target = swapchain.getBuffer(i) % std::format(L"frame-{}", i);
        bindRtv(target, rtvCpuHandle(i));

        frameData[i] = {
            .value = 0,
            .target = target
        };
    }
}

void Context::destroyFrameData() {
    sceneTarget.tryDrop("scene-target");

    const auto count = getBufferCount();
    for (size_t i = 0; i < count; i++) {
        frameData[i].target.release(); // render targets share a reference counter
    }
}

void Context::createFence() {
    frameData[frameIndex].value = 1;
    fence = device.newFence(0) % "fence";
    fenceEvent = CreateEvent(nullptr, false, false, TEXT("fence-event"));
    if (!fenceEvent) { throw win32::Error("CreateEvent"); }
}

void Context::destroyFence() {
    CloseHandle(fenceEvent);
    fence.tryDrop("fence");
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

    postResolution = Resolution(displayWidth, displayHeight);
    sceneResolution = Resolution(internalWidth, internalHeight);
}
