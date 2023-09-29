#pragma once

#include "simcoe/rhi/rhi.h"
#include "simcoe/core/system.h"

#include "dx/d3d12.h"
#include <dxgi1_6.h>
#include <dxgidebug.h>

namespace rhi = simcoe::rhi;
namespace math = simcoe::math;

#define RELEASE(p) do { if (p != nullptr) { p->Release(); p = nullptr; } } while (0)

namespace dxgi {
    using Factory = IDXGIFactory5;
    using SimpleAdapter = IDXGIAdapter1;
    using Adapter = IDXGIAdapter4;
}

namespace d3d {
    using Device = ID3D12Device4;
    using Queue = ID3D12CommandQueue;
    using DescriptorHeap = ID3D12DescriptorHeap;
}

template<typename T>
struct ComObject {
    ComObject(T *pObject)
        : pObject(pObject)
    { }

    ~ComObject() {
        RELEASE(pObject);
    }

    T *get() const { return pObject; }

    T **operator&() { return &pObject; }
    T *operator->() const { return pObject; }
    T *operator*() const { return pObject; }
protected:
    T *pObject = nullptr;
};

template<typename T, typename S>
struct RenderObject : ComObject<T>, S {
    using O = ComObject<T>;
    using S::S;

    template<typename... Args>
    RenderObject(T *pObject, Args&&... args)
        : O(pObject)
        , S(std::forward<Args>(args)...)
    { }
};

struct DxFence final : RenderObject<ID3D12Fence, rhi::IFence> {
    using Super = RenderObject<ID3D12Fence, rhi::IFence>;
    using Super::Super;

    // public interface

    void wait(size_t value) override;
    size_t getValue() const override;

    // module interface

    static DxFence *create(d3d::Device *pDevice);

private:
    DxFence(ID3D12Fence *pObject, HANDLE event)
        : Super(pObject)
        , event(event)
    { }

    HANDLE event;
};

struct DxHeap final : RenderObject<d3d::DescriptorHeap, rhi::IHeap> {
    using Super = RenderObject<d3d::DescriptorHeap, rhi::IHeap>;
    using Super::Super;

    // public interface

    rhi::GpuHandle getGpuHandle(size_t index) override;
    rhi::CpuHandle getCpuHandle(size_t index) override;

    static DxHeap *create(d3d::Device *pDevice, D3D12_DESCRIPTOR_HEAP_TYPE type, size_t size);

private:
    DxHeap(d3d::DescriptorHeap *pHeap, UINT increment)
        : Super(pHeap)
        , increment(increment)
    { }

    UINT increment = 0;
};

struct DxSurface final : RenderObject<ID3D12Resource, rhi::ISurface> {
    using Super = RenderObject<ID3D12Resource, rhi::ISurface>;
    using Super::Super;

    // module interface

    static DxSurface *create(ID3D12Resource *pResource);
};

struct DxDisplayQueue final : RenderObject<IDXGISwapChain4, rhi::IDisplayQueue> {
    using Super = RenderObject<IDXGISwapChain4, rhi::IDisplayQueue>;
    using Super::Super;

    // public interface

    void present() override;
    size_t getCurrentFrame() const override;
    rhi::ISurface *getSurface(size_t index) override;

    // module interface

    static DxDisplayQueue *create(IDXGISwapChain4 *pSwapChain, BOOL tearing);

private:
    DxDisplayQueue(IDXGISwapChain4 *pObject, BOOL tearing)
        : Super(pObject)
        , tearingSupport(tearing)
    { }

    BOOL tearingSupport = FALSE;
};

struct DxCommandList final : RenderObject<ID3D12GraphicsCommandList, rhi::ICommandList> {
    using Super = RenderObject<ID3D12GraphicsCommandList, rhi::ICommandList>;
    using Super::Super;

    // public interface

    void begin() override;
    void end() override;

    void clearRenderTarget(rhi::CpuHandle handle, math::float4 colour) override;

    void transition(std::span<const rhi::Barrier> barriers) override;

    // module interface

    static DxCommandList *create(d3d::Device *pDevice, D3D12_COMMAND_LIST_TYPE type);

private:
    DxCommandList(ID3D12GraphicsCommandList *pObject, ID3D12CommandAllocator *pAlloc)
        : Super(pObject)
        , pAllocator(pAlloc)
    { }

    ComObject<ID3D12CommandAllocator> pAllocator;
};

struct DxCommandQueue final : RenderObject<d3d::Queue, rhi::ICommandQueue> {
    using Super = RenderObject<d3d::Queue, rhi::ICommandQueue>;
    using Super::Super;

    // public interface

    rhi::IDisplayQueue *createDisplayQueue(rhi::IContext *ctx, const rhi::DisplayQueueInfo& info) override;

    void execute(std::span<rhi::ICommandList*> lists) override;
    void signal(rhi::IFence *pFence, size_t value) override;

    // module interface

    static DxCommandQueue *create(d3d::Device *pDevice, D3D12_COMMAND_LIST_TYPE type);
};

struct DxDevice final : RenderObject<d3d::Device, rhi::IDevice> {
    using Super = RenderObject<d3d::Device, rhi::IDevice>;
    using Super::Super;

    // public interface

    rhi::ICommandQueue *createCommandQueue(rhi::CommandType type) override;
    rhi::ICommandList *createCommandList(rhi::CommandType type) override;
    rhi::IHeap *createHeap(rhi::HeapType type, size_t size) override;
    rhi::IFence *createFence() override;

    void mapRenderTarget(rhi::ISurface *pSurface, rhi::CpuHandle handle) override;

    // module interface

    static DxDevice *create(dxgi::Factory *pFactory, dxgi::Adapter *pAdapter);

private:
    DxDevice(dxgi::Factory *pFactory, d3d::Device *pDevice)
        : Super(pDevice)
        , pFactory(pFactory)
    { }

    dxgi::Factory *pFactory = nullptr;
};

struct DxAdapter final : RenderObject<dxgi::Adapter, rhi::IAdapter> {
    using Super = RenderObject<dxgi::Adapter, rhi::IAdapter>;
    using Super::Super;

    // public interface

    rhi::IDevice *createDevice() override;

    // module interface

    static DxAdapter *create(dxgi::Factory *pFactory, dxgi::SimpleAdapter *pAdapter1);
private:
    DxAdapter(dxgi::Factory *pFactory, dxgi::Adapter *pAdapter, const rhi::AdapterInfo& info)
        : Super(pAdapter, info)
        , pFactory(pFactory)
    { }

    dxgi::Factory *pFactory = nullptr;
};

struct DxContext final : RenderObject<dxgi::Factory, rhi::IContext> {
    using Super = RenderObject<dxgi::Factory, rhi::IContext>;
    using Super::Super;

    rhi::IAdapter *getAdapter(size_t index) override;

    static DxContext *create();
};
