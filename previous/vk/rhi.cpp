#include "common.h"

#include <array>

using ResultType = std::underlying_type_t<VkResult>;

#define VX_CHECK(expr) \
    do { \
        if (VkResult err = (expr); err != VK_SUCCESS) { \
            PANIC("{} = {}", #expr, ResultType(err)); \
        } \
    } while (false)


namespace {
    constexpr auto kInstanceExtensions = std::to_array<const char*>({
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME
    });

    constexpr auto kDeviceExtensions = std::to_array<const char*>({
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    });

    constexpr rhi::AdapterType getAdapterType(VkPhysicalDeviceType type) {
        switch (type) {
        case VK_PHYSICAL_DEVICE_TYPE_CPU:
            return rhi::AdapterType::eSoftware;

        default:
            return rhi::AdapterType::eNone;
        }
    }

    std::vector<VkQueueFamilyProperties> getQueueFamilies(VkPhysicalDevice physicalDevice) {
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

        return queueFamilies;
    }

    uint32_t findQueueFamily(std::span<VkQueueFamilyProperties> queueFamilies, VkQueueFlags flags) {
        for (uint32_t i = 0; i < queueFamilies.size(); ++i) {
            if (queueFamilies[i].queueFlags & flags) {
                return i;
            }
        }

        NEVER("failed to find queue family");
    }

    QueueFamilies findAllQueueFamilies(std::span<VkQueueFamilyProperties> queueFamilies) {
        QueueFamilies result = {
            .graphics = findQueueFamily(queueFamilies, VK_QUEUE_GRAPHICS_BIT),
            .present = findQueueFamily(queueFamilies, VK_QUEUE_GRAPHICS_BIT),
            .copy = findQueueFamily(queueFamilies, VK_QUEUE_GRAPHICS_BIT)
        };

        return result;
    }

    uint32_t getQueueIndex(const QueueFamilies& indicies, rhi::CommandType type) {
        switch (type) {
        case rhi::CommandType::ePresent:
            return indicies.present;
        case rhi::CommandType::eDirect:
            return indicies.graphics;
        case rhi::CommandType::eCopy:
            return indicies.copy;

        default:
            NEVER("invalid command type");
        }
    }
}

// command list

void VxCommandList::begin() {
    VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };

    VX_CHECK(vkBeginCommandBuffer(commands, &beginInfo));
}

void VxCommandList::end() {
    vkResetCommandBuffer(commands, 0);
}

void VxCommandList::transition(std::span<const rhi::Barrier>) {

}

void VxCommandList::clearRenderTarget(rhi::CpuHandle, math::float4) {

}

// display queue

void VxDisplayQueue::present() {

}

size_t VxDisplayQueue::getCurrentFrame() const {
    return SIZE_MAX;
}

rhi::ISurface *VxDisplayQueue::getSurface(size_t) {
    return nullptr;
}

// queue

rhi::IDisplayQueue *VxQueue::createDisplayQueue(rhi::IContext *ctx, rhi::IDevice *device, const rhi::DisplayQueueInfo& info) {
    VxContext *pContext = static_cast<VxContext*>(ctx);
    VxDevice *pDevice = static_cast<VxDevice*>(device);

    VkWin32SurfaceCreateInfoKHR createInfo = {
        .sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
        .hinstance = GetModuleHandle(nullptr),
        .hwnd = info.window.getHandle()
    };

    VkSurfaceKHR surface;
    VX_CHECK(vkCreateWin32SurfaceKHR(pContext->getInstance(), &createInfo, nullptr, &surface));

    uint32_t presentQueue = pDevice->getPresentQueue(surface);

    VkBool32 presentSupport = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(pDevice->getPhysicalDevice(), presentQueue, surface, &presentSupport);

    // VkDeviceQueueCreateInfo queueCreateInfo = {
    //     .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
    //     .queueFamilyIndex = presentQueue,
    //     .queueCount = 1,
    //     .pQueuePriorities = nullptr
    // };

    // VkQueue queue;
    // vkGetDeviceQueue(pDevice->getDevice(), presentQueue, 0, &queue);

    return VxDisplayQueue::create(nullptr, surface, nullptr);
}

void VxQueue::execute(std::span<rhi::ICommandList*>) {

}

void VxQueue::signal(rhi::IFence *, size_t) {

}

// device

rhi::ICommandQueue *VxDevice::createCommandQueue(rhi::CommandType type) {
    VkQueue queue;
    vkGetDeviceQueue(device, getQueueIndex(queueFamilies, type), 0, &queue);

    return VxQueue::create(queue);
}

rhi::ICommandList *VxDevice::createCommandList(rhi::CommandType) {
    return nullptr;
}

rhi::IHeap *VxDevice::createHeap(rhi::HeapType, size_t) {
    return nullptr;
}

rhi::IFence *VxDevice::createFence() {
    return nullptr;
}

void VxDevice::mapRenderTarget(rhi::ISurface *, rhi::CpuHandle ) {

}

uint32_t VxDevice::getPresentQueue(VkSurfaceKHR surface) const {
    for (uint32_t i = 0; i < queueFamilyProperties.size(); ++i) {
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupport);

        if (presentSupport) {
            return i;
        }
    }

    return UINT32_MAX;
}

VxDevice *VxDevice::create(VkPhysicalDevice physicalDevice) {
    VkDeviceCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = nullptr,
        .enabledExtensionCount = uint32_t(kDeviceExtensions.size()),
        .ppEnabledExtensionNames = kDeviceExtensions.data(),
    };

    VkDevice device;
    VX_CHECK(vkCreateDevice(physicalDevice, &createInfo, nullptr, &device));

    auto queues = getQueueFamilies(physicalDevice);

    QueueFamilies families = findAllQueueFamilies(queues);

    return new VxDevice(device, queues, physicalDevice, families);
}

// adapter

rhi::IDevice *VxAdapter::createDevice() {
    return VxDevice::create(physicalDevice);
}

VxAdapter *VxAdapter::create(VkPhysicalDevice physicalDevice) {
    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(physicalDevice, &properties);

    rhi::AdapterInfo info = {
        .name = properties.deviceName,
        .type = getAdapterType(properties.deviceType)
    };

    return new VxAdapter(physicalDevice, info);
}

// context

rhi::IAdapter *VxContext::getAdapter(size_t index) {
    if (index >= devices.size()) { return nullptr; }

    return VxAdapter::create(devices[index]);
}

VxContext *VxContext::create() {
    VkInstanceCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = nullptr,
        .enabledExtensionCount = uint32_t(kInstanceExtensions.size()),
        .ppEnabledExtensionNames = kInstanceExtensions.data(),
    };

    VkInstance instance;
    VX_CHECK(vkCreateInstance(&createInfo, nullptr, &instance));

    uint32_t deviceCount = 0;
    VX_CHECK(vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr));

    std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
    VX_CHECK(vkEnumeratePhysicalDevices(instance, &deviceCount, physicalDevices.data()));

    return new VxContext(instance, physicalDevices);
}

extern "C" DLLEXPORT rhi::IContext *rhiGetContext() {
    return VxContext::create();
}
