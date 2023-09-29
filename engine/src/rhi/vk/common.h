#pragma once

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

#include "simcoe/rhi/rhi.h"

namespace rhi = simcoe::rhi;
namespace math = simcoe::math;

struct QueueFamilies {
    uint32_t graphics;
    uint32_t present;
    uint32_t copy;
};

struct VxInstance;
struct VxAdapter;
struct VxDevice;

struct VxDisplayQueue final : rhi::IDisplayQueue {
    // public interface
    void present() override;
    size_t getCurrentFrame() const override;
    rhi::ISurface *getSurface(size_t index) override;

    static VxDisplayQueue *create(VkQueue presentQueue, VkSurfaceKHR surface, VkSwapchainKHR swapchain);

private:
    VxDisplayQueue(VkSwapchainKHR swapchain)
        : swapchain(swapchain)
    { }

    VkSwapchainKHR swapchain;
};

struct VxCommandList final : rhi::ICommandList {
    // public interface
    void begin() override;
    void end() override;

    void transition(std::span<const rhi::Barrier> barriers) override;

    void clearRenderTarget(rhi::CpuHandle handle, math::float4 colour) override;

    // module interface

    static VxCommandList *create(VkCommandBuffer commandBuffer);

private:
    VxCommandList(VkCommandBuffer commandBuffer)
        : commands(commandBuffer)
    { }

    VkCommandBuffer commands;
};

struct VxDevice final : rhi::IDevice {
    rhi::ICommandQueue *createCommandQueue(rhi::CommandType type) override;
    rhi::ICommandList *createCommandList(rhi::CommandType type) override;
    rhi::IHeap *createHeap(rhi::HeapType type, size_t size) override;
    rhi::IFence *createFence() override;

    void mapRenderTarget(rhi::ISurface *pSurface, rhi::CpuHandle handle) override;

    // module interface

    VkDevice getDevice() const { return device; }
    VkPhysicalDevice getPhysicalDevice() const { return physicalDevice; }
    uint32_t getPresentQueue(VkSurfaceKHR surface) const;

    static VxDevice *create(VkPhysicalDevice physicalDevice);

private:
    VxDevice(VkDevice device, std::vector<VkQueueFamilyProperties> queueFamilyProperties, VkPhysicalDevice physicalDevice, QueueFamilies queueFamilies)
        : device(device)
        , queueFamilyProperties(queueFamilyProperties)
        , physicalDevice(physicalDevice)
        , queueFamilies(queueFamilies)
    { }

    VkDevice device;
    std::vector<VkQueueFamilyProperties> queueFamilyProperties;
    VkPhysicalDevice physicalDevice;
    QueueFamilies queueFamilies;
};

struct VxQueue final : rhi::ICommandQueue {
    // public interface
    rhi::IDisplayQueue *createDisplayQueue(rhi::IContext *ctx, rhi::IDevice *device, const rhi::DisplayQueueInfo& info) override;

    void execute(std::span<rhi::ICommandList*> lists) override;
    void signal(rhi::IFence *pFence, size_t value) override;

    // module interface

    static VxQueue *create(VkQueue queue);

private:
    VxQueue(VkQueue queue)
        : queue(queue)
    { }

    VkQueue queue;
};

struct VxAdapter final : rhi::IAdapter {
    rhi::IDevice *createDevice() override;

    static VxAdapter *create(VkPhysicalDevice physicalDevice);

private:
    VxAdapter(VkPhysicalDevice physicalDevice, const rhi::AdapterInfo& info)
        : rhi::IAdapter(info)
        , physicalDevice(physicalDevice)
    { }

    VkPhysicalDevice physicalDevice;
};

struct VxContext final : rhi::IContext {
    // public interface
    rhi::IAdapter *getAdapter(size_t index) override;

    // module interface

    VkInstance getInstance() const { return instance; }

    static VxContext *create();

private:
    VxContext(VkInstance instance, std::vector<VkPhysicalDevice> devices)
        : instance(instance)
        , devices(devices)
    { }

    VkInstance instance;
    std::vector<VkPhysicalDevice> devices;
};
