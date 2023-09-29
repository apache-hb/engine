#pragma once

#include <string>
#include <span>

#include "simcoe/core/system.h"

namespace simcoe::rhi {
    // fwd

    struct IContext;
    struct IAdapter;
    struct IDevice;

    struct IDisplayQueue; // a swapchain

    struct ICommandQueue;
    struct ICommandList;

    struct IFence;
    struct IHeap;
    struct ISurface;

    // pod types

    enum struct AdapterType {
        eNone,
        eSoftware,
    };

    enum struct GpuHandle : size_t { eInvalid = SIZE_MAX };
    enum struct CpuHandle : size_t { eInvalid = SIZE_MAX };

    enum struct ColourFormat {
        eRGBA8
    };

    enum struct CommandType {
        ePresent,
        eDirect,
        eCopy
    };

    enum struct ResourceState {
        ePresent,
        eRenderTarget
    };

    enum struct BarrierType {
        eTransition
    };

    enum struct HeapType {
        eDepthStencil,
        eRenderTarget
    };

    struct AdapterInfo {
        std::string name;
        AdapterType type;
    };

    struct DisplayQueueInfo {
        os::Window& window;
        os::Size size;
        ColourFormat format;
        size_t frames;
    };

    struct Transition {
        rhi::ISurface *pResource;
        ResourceState from;
        ResourceState to;
    };

    struct Barrier {
        BarrierType type;
        union Data {
            Transition transition;
        } data;
    };

    struct Viewport {
        math::float2 topLeft;
        math::float2 size;
        float minDepth;
        float maxDepth;
    };

    struct Rect {
        int left;
        int top;
        int right;
        int bottom;
    };

    Barrier transition(rhi::ISurface *pResource, ResourceState from, ResourceState to);

    // interface types

    struct IContext {
        virtual ~IContext() = default;

        virtual IAdapter *getAdapter(size_t index) = 0;
    };

    struct IAdapter {
        virtual ~IAdapter() = default;

        const AdapterInfo& getInfo() const { return info; }
        virtual IDevice *createDevice() = 0;

    protected:
        IAdapter(const AdapterInfo &info) : info(info) { }

        AdapterInfo info;
    };

    struct IDevice {
        virtual ~IDevice() = default;

        virtual ICommandQueue *createCommandQueue(CommandType type) = 0;
        virtual ICommandList *createCommandList(CommandType type) = 0;
        virtual IHeap *createHeap(HeapType type, size_t size) = 0;
        virtual IFence *createFence() = 0;

        virtual void mapRenderTarget(ISurface *pSurface, rhi::CpuHandle handle) = 0;
    };

    struct IDisplayQueue {
        virtual ~IDisplayQueue() = default;

        virtual void present() = 0;
        virtual size_t getCurrentFrame() const = 0;
        virtual ISurface *getSurface(size_t index) = 0;
    };

    struct IFence {
        virtual ~IFence() = default;

        virtual void wait(size_t value) = 0;
        virtual size_t getValue() const = 0;
    };

    struct ICommandQueue {
        virtual ~ICommandQueue() = default;

        virtual IDisplayQueue *createDisplayQueue(IContext *context, const DisplayQueueInfo& info) = 0;

        virtual void execute(std::span<ICommandList*> lists) = 0;
        virtual void signal(IFence *pFence, size_t value) = 0;
    };

    struct ICommandList {
        virtual ~ICommandList() = default;

        virtual void begin() = 0;
        virtual void end() = 0;

        virtual void clearRenderTarget(CpuHandle handle, math::float4 colour) = 0;

        virtual void transition(std::span<const Barrier> barriers) = 0;
    };

    struct IHeap {
        virtual ~IHeap() = default;

        virtual GpuHandle getGpuHandle(size_t index) = 0;
        virtual CpuHandle getCpuHandle(size_t index) = 0;
    };

    struct ISurface {
        virtual ~ISurface() = default;
    };

    using CreateContext = rhi::IContext*(*)(void);
}
