#include "render/render.h"

#include <array>

namespace engine::render {
    constexpr float kBorderColour[] = { 0.f, 0.f, 0.f, 1.f };

    Context::Context(Context::Create&& create): info(create) {
        buildViews();
        createContext();
        createDevice();
        createBuffers();
        createCopyPipeline();
        createPipeline();
        createScene();
    }

    Context::~Context() {
        destroyCopyPipeline();
        destroyScene();
        destroyPipeline();
        destroyBuffers(); 
        destroyDevice();
        destroyContext();
    }

    void Context::populate() {
        auto index = getCurrentFrameIndex();
        auto target = frameData[index].target.get();
        auto intermediate = intermediateRenderTarget;
        auto rtvHandle = getRenderTargetCpuHandle(index);
        ID3D12DescriptorHeap *heaps[] = { cbvHeap.get() };

        D3D12_RESOURCE_BARRIER inTransitions[] = {
            CD3DX12_RESOURCE_BARRIER::Transition(target, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET),
            CD3DX12_RESOURCE_BARRIER::Transition(intermediate.get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE)
        };

        D3D12_RESOURCE_BARRIER outTransitions[] = {
            CD3DX12_RESOURCE_BARRIER::Transition(target, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT),
            CD3DX12_RESOURCE_BARRIER::Transition(intermediate.get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET)
        };

        directCommands->ResourceBarrier(UINT(std::size(inTransitions)), inTransitions);

        directCommands->RSSetScissorRects(1, &postView.scissor);
        directCommands->RSSetViewports(1, &postView.viewport);

        directCommands->OMSetRenderTargets(1, &rtvHandle, false, nullptr);
        directCommands->ClearRenderTargetView(rtvHandle, kBorderColour, 0, nullptr);

        directCommands->SetDescriptorHeaps(UINT(std::size(heaps)), heaps);
        directCommands->SetGraphicsRootSignature(rootSignature.get());
        directCommands->SetGraphicsRootDescriptorTable(0, getIntermediateCbvGpuHandle());

        directCommands->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
        directCommands->IASetVertexBuffers(0, 1, &screenBuffer.view);
        directCommands->DrawInstanced(4, 1, 0, 0);

        directCommands->ResourceBarrier(UINT(std::size(outTransitions)), outTransitions);
    }

    bool Context::present() noexcept {
        try {
            directCommands.reset(getAllocator(Allocator::Direct).get(), pipelineState);
            
            populate();
            
            directCommands.close();

            auto commands = std::to_array({ directCommands });
            directCommandQueue.execute({ commands });

            nextFrame(directCommandQueue);
            return true;
        } catch (const engine::Error& error) {
            log::render->fatal("failed to present: {}", error.query());
        }

        return false;
    }

    void Context::waitForGpu(Queue queue) {
        UINT64 &value = frameData[getCurrentFrameIndex()].fenceValue;
        check(queue->Signal(fence.get(), value), "failed to signal fence");

        check(fence->SetEventOnCompletion(value, fenceEvent), "failed to set fence event");
        WaitForSingleObject(fenceEvent, INFINITE);

        value += 1;
    }

    void Context::nextFrame(Queue queue) {
        UINT64 value = frameData[getCurrentFrameIndex()].fenceValue;
        check(queue->Signal(fence.get(), value), "failed to signal fence");
        
        frameIndex = swapchain->GetCurrentBackBufferIndex();

        auto &current = frameData[getCurrentFrameIndex()].fenceValue;

        if (fence->GetCompletedValue() < current) {
            check(fence->SetEventOnCompletion(current, fenceEvent), "failed to set fence event");
            WaitForSingleObject(fenceEvent, INFINITE);
        }

        current = value + 1;
    }
}
