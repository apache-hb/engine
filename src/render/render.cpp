#include "render.h"

namespace engine::render {
    HRESULT Context::createCore() {
        auto size = window->getClientSize();
        if (!size) { return E_FAIL; }

        auto [width, height] = size.value();
        auto handle = window->getHandle();

        auto adapter = getAdapter();
        if (HRESULT hr = D3D12CreateDevice(adapter.get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device)); FAILED(hr)) {
            render->fatal("failed to create device\n{}", to_string(hr));
            return hr;
        }

        {
            D3D12_COMMAND_QUEUE_DESC desc = {
                .Type = D3D12_COMMAND_LIST_TYPE_DIRECT
            };
            if (HRESULT hr = device->CreateCommandQueue(&desc, IID_PPV_ARGS(&commandQueue)); FAILED(hr)) {
                render->fatal("failed to create command queue\n{}", to_string(hr));
                return hr;
            }
        }

        {
            dxgi::SwapChain1 swapchain1;
            DXGI_SWAP_CHAIN_DESC1 desc = {
                .Width = UINT(width),
                .Height = UINT(height),
                .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
                .SampleDesc = { .Count = 1 },
                .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
                .BufferCount = frameCount,
                .SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD
            };

            if (HRESULT hr = factory->CreateSwapChainForHwnd(commandQueue.get(), handle, &desc, nullptr, nullptr, &swapchain1); FAILED(hr)) {
                render->fatal("failed to create swap chain\n{}", to_string(hr));
                return hr;
            }

            if (HRESULT hr = factory->MakeWindowAssociation(handle, DXGI_MWA_NO_ALT_ENTER); FAILED(hr)) {
                render->fatal("failed to make window association\n{}", to_string(hr));
                return hr;
            }

            if (auto swap = swapchain1.as<dxgi::SwapChain3>(); !swap) {
                render->fatal("failed to create swap chain\n{}", to_string(swap.error()));
                return swap.error();
            } else {
                swapchain = swap.value();
                swapchain.release(); /// make sure only one reference is held
            }

            frameIndex = swapchain->GetCurrentBackBufferIndex();
        }

        scissor = d3d12::Scissor(LONG(width), LONG(height));
        viewport = d3d12::Viewport(FLOAT(width), FLOAT(height));

        {
            D3D12_DESCRIPTOR_HEAP_DESC desc = {
                .Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
                .NumDescriptors = frameCount
            };

            if (HRESULT hr = device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&rtvHeap)); FAILED(hr)) {
                render->fatal("failed to create rtv heap\n{}", to_string(hr));
                return hr;
            }

            rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        }

        {
            D3D12_DESCRIPTOR_HEAP_DESC desc = {
                .Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
                .NumDescriptors = 1,
                .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
            };

            if (HRESULT hr = device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&srvHeap)); FAILED(hr)) {
                render->fatal("failed to create srv heap\n{}", to_string(hr));
                return hr;
            }
        }

        {
            renderTargets = new d3d12::Resource[frameCount];
            CD3DX12_CPU_DESCRIPTOR_HANDLE handle(rtvHeap->GetCPUDescriptorHandleForHeapStart());

            for (UINT i = 0; i < frameCount; i++) {
                if (HRESULT hr = swapchain->GetBuffer(i, IID_PPV_ARGS(&renderTargets[i])); FAILED(hr)) {
                    render->fatal("failed to get buffer\n{}", to_string(hr));
                    return hr;
                }

                device->CreateRenderTargetView(renderTargets[i].get(), nullptr, handle);

                handle.Offset(1, rtvDescriptorSize);
            }
        }

        if (HRESULT hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator)); FAILED(hr)) {
            render->fatal("failed to create command allocator\n{}", to_string(hr));
            return hr;
        }

        return S_OK;
    }

    void Context::destroyCore() {
        commandAllocator.tryDrop("command-allocator");
        
        UINT remaining = 0;
        for (UINT i = 0; i < frameCount; i++) {
            remaining = renderTargets[i].release(); // render targets all share a reference counter
        }
        delete[] renderTargets;
        
        if (remaining != 0) {
            render->fatal("failed to release all render targets, {} references remaining", remaining);
        }

        srvHeap.tryDrop("srv-heap");
        rtvHeap.tryDrop("rtv-heap");
        swapchain.tryDrop("swapchain");
        commandQueue.tryDrop("command-queue");
        device.tryDrop("device");
    }

    HRESULT Context::createAssets() {
        if (HRESULT hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator.get(), nullptr, IID_PPV_ARGS(&commandList)); FAILED(hr)) {
            render->fatal("failed to create command list\n{}", to_string(hr));
            return hr;
        }

        if (HRESULT hr = commandList->Close(); FAILED(hr)) {
            render->fatal("failed to close command list\n{}", to_string(hr));
            return hr;
        }

        if (HRESULT hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)); FAILED(hr)) {
            render->fatal("failed to create fence\n{}", to_string(hr));
            return hr;
        }
        fenceValue = 1;

        fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (fenceEvent == nullptr) {
            HRESULT hr = HRESULT_FROM_WIN32(GetLastError());
            render->fatal("failed to create fence event\n{}", to_string(hr));
            return hr;
        }

        ImGui::CreateContext();
        ImGui::GetIO();
        ImGui::StyleColorsDark();

        ImGui_ImplWin32_Init(window->getHandle());
        ImGui_ImplDX12_Init(
            device.get(), 
            frameCount,
            DXGI_FORMAT_R8G8B8A8_UNORM,
            srvHeap.get(),
            srvHeap->GetCPUDescriptorHandleForHeapStart(),
            srvHeap->GetGPUDescriptorHandleForHeapStart()
        );

        return S_OK;
    }

    void Context::destroyAssets() {  
        ImGui_ImplDX12_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();

        CloseHandle(fenceEvent);
        fence.tryDrop("fence");
        commandList.tryDrop("command-list");
    }

    HRESULT Context::present() {
        ImGui_ImplDX12_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        ImGui::ShowDemoWindow();

        ImGui::Render();

        if (HRESULT hr = populate(); FAILED(hr)) {
            retry();
            return hr;
        }
        
        ID3D12CommandList *commands[] = { commandList.get() };
        commandQueue->ExecuteCommandLists(1, commands);

        if (HRESULT hr = swapchain->Present(0, 0); FAILED(hr)) {
            retry();
            return hr;
        }

        if (HRESULT hr = waitForFrame(); FAILED(hr)) {
            retry();
            return hr;
        }

        return S_OK;
    }

    HRESULT Context::populate() {
        if (HRESULT hr = commandAllocator->Reset(); FAILED(hr)) {
            render->fatal("failed to reset command allocator\n{}", to_string(hr));
            return hr;
        }

        if (HRESULT hr = commandList->Reset(commandAllocator.get(), nullptr); FAILED(hr)) {
            render->fatal("failed to reset command list\n{}", to_string(hr));
            return hr;
        }

        const auto toTarget = CD3DX12_RESOURCE_BARRIER::Transition(
            renderTargets[frameIndex].get(),
            D3D12_RESOURCE_STATE_PRESENT,
            D3D12_RESOURCE_STATE_RENDER_TARGET
        );

        const auto toPresent = CD3DX12_RESOURCE_BARRIER::Transition(
            renderTargets[frameIndex].get(),
            D3D12_RESOURCE_STATE_RENDER_TARGET,
            D3D12_RESOURCE_STATE_PRESENT
        );

        commandList->ResourceBarrier(1, &toTarget);

        CD3DX12_CPU_DESCRIPTOR_HANDLE handle(rtvHeap->GetCPUDescriptorHandleForHeapStart(), frameIndex, rtvDescriptorSize);

        const float clear[] = { 0.0f, 0.2f, 0.4f, 1.0f };
        commandList->ClearRenderTargetView(handle, clear, 0, nullptr);
        commandList->OMSetRenderTargets(1, &handle, false, nullptr);
        commandList->SetDescriptorHeaps(1, &srvHeap);

        ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList.get());

        commandList->ResourceBarrier(1, &toPresent);

        if (HRESULT hr = commandList->Close(); FAILED(hr)) {
            render->fatal("failed to close command list\n{}", to_string(hr));
            return hr;
        }

        return S_OK;
    }

    HRESULT Context::waitForFrame() {
        UINT64 value = fenceValue;
        if (HRESULT hr = commandQueue->Signal(fence.get(), value); FAILED(hr)) {
            render->fatal("failed to signal fence\n{}", to_string(hr));
            return hr;
        }
        fenceValue += 1;

        if (fence->GetCompletedValue() < value) {
            if (HRESULT hr = fence->SetEventOnCompletion(value, fenceEvent); FAILED(hr)) {
                render->fatal("failed to set fence event\n{}", to_string(hr));
                return hr;
            }
            WaitForSingleObject(fenceEvent, INFINITE);
        }

        frameIndex = swapchain->GetCurrentBackBufferIndex();

        return S_OK;
    }

    void Context::retry() {
        HRESULT hr = device->GetDeviceRemovedReason();
        render->warn("device removed\n{}", to_string(hr));

        destroyAssets();
        destroyCore();
        createCore();
        createAssets();
    }

    Context createContext(Factory factory, system::Window *window, size_t adapter, UINT frameCount) {
        IMGUI_CHECKVERSION();

        return Context(factory, adapter, window, frameCount);
    }
}
