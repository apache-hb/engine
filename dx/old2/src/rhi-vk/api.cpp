#include "engine/rhi/api.h"

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

#include <vector>
#include <array>
#include <string>

#include "engine/base/panic.h"

using namespace engine;

namespace {
    constexpr VkApplicationInfo kAppInfo {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "game",
        .applicationVersion = VK_MAKE_VERSION(0, 0, 1),
        .pEngineName = "engine",
        .engineVersion = VK_MAKE_VERSION(0, 0, 1),
        .apiVersion = VK_API_VERSION_1_0
    };

    constexpr VkPhysicalDeviceFeatures kRequiredFeatures {

    };

    constexpr auto kRequiredExtensions = std::to_array({
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
        VK_EXT_DEBUG_REPORT_EXTENSION_NAME
    });

    constexpr auto kRequiredLayers = std::to_array({
        "VK_LAYER_KHRONOS_validation"
    });

    constexpr float kHighestQueuePriority = 1.f;

    constexpr std::string vkErrorString(VkResult result) {
        switch (result) {
        case VK_SUCCESS: return "VK_SUCCESS";
        case VK_ERROR_EXTENSION_NOT_PRESENT: return "VK_ERROR_EXTENSION_NOT_PRESENT";
        default: return fmt::format("VkResult({})", signed(result));
        }
    }
}

#define VK_CHECK(expr) do { if (VkResult result = (expr); result != VK_SUCCESS) { ASSERTF(false, #expr " => {}", vkErrorString(result)); } } while (0)

struct rhiDevice final : rhi::Device {
    rhiDevice(VkDevice device, VkQueue graphicsQueue)
        : device(device)
        , graphicsQueue(graphicsQueue)
    { }

private:
    VkDevice device;
    VkQueue graphicsQueue;
};

struct rhiAdapter final : rhi::Adapter {
    rhiAdapter(VkPhysicalDevice device): device(device) { 
        vkGetPhysicalDeviceProperties(device, &props);
        collectQueues();
        findRequiredQueues();
    }

    std::string_view getName() const override {
        return props.deviceName;
    }

    rhi::Adapter::Type getType() const override {
        switch (props.deviceType) {
        case VK_PHYSICAL_DEVICE_TYPE_CPU: 
            return rhi::Adapter::eSoftare;

        case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
        case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
        case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
            return rhi::Adapter::eDiscrete;

        default: 
            return rhi::Adapter::eOther;
        }
    }

    VkPhysicalDevice get() { return device; }

    uint32_t getGraphicsQueue() { return uint32_t(graphicsQueueIndex); }
private:
    void collectQueues() {
        uint32_t count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &count, nullptr);

        queues.resize(count);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &count, queues.data());
    }

    void findRequiredQueues() {
        for (size_t i = 0; i < queues.size(); i++) {
            VkQueueFamilyProperties queue = queues[i];
            if (queue.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                graphicsQueueIndex = i;
            }
        }
    }

    VkPhysicalDevice device;

    size_t graphicsQueueIndex = SIZE_MAX;

    std::vector<VkQueueFamilyProperties> queues;
    VkPhysicalDeviceProperties props;
};

struct rhiInstance final : rhi::Instance {
    rhiInstance(VkInstance instance): instance(instance) { 
        collectAdapters();
    }

    rhi::Device *newDevice(rhi::Adapter *adapter) override {
        auto *it = static_cast<rhiAdapter*>(adapter);

        const uint32_t kGraphicsQueueIndex = it->getGraphicsQueue();

        const VkDeviceQueueCreateInfo kQueueInfo {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = kGraphicsQueueIndex,
            .queueCount = 1,
            .pQueuePriorities = &kHighestQueuePriority
        };

        const VkDeviceCreateInfo kCreateInfo {
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .queueCreateInfoCount = 1,
            .pQueueCreateInfos = &kQueueInfo,

            .enabledLayerCount = uint32_t(kRequiredLayers.size()),
            .ppEnabledLayerNames = kRequiredLayers.data(),

            .enabledExtensionCount = uint32_t(kRequiredExtensions.size()),
            .ppEnabledExtensionNames = kRequiredExtensions.data(),

            .pEnabledFeatures = &kRequiredFeatures
        };

        VkDevice device;
        VkQueue graphicsQueue;

        VK_CHECK(vkCreateDevice(it->get(), &kCreateInfo, nullptr, &device));

        vkGetDeviceQueue(device, kGraphicsQueueIndex, 0, &graphicsQueue);

        return new rhiDevice(device, graphicsQueue);
    }

    std::span<rhi::Adapter*> getAdapters() override {
        return devices;
    }

    ~rhiInstance() override {
        vkDestroyInstance(instance, nullptr);
    }

private:
    void collectAdapters() {
        uint32_t count = 0;
        vkEnumeratePhysicalDevices(instance, &count, nullptr);

        auto physicalDevices = std::unique_ptr<VkPhysicalDevice[]>(new VkPhysicalDevice[count]);
        vkEnumeratePhysicalDevices(instance, &count, physicalDevices.get());

        for (size_t i = 0; i < count; i++) {
            devices.push_back(new rhiAdapter(physicalDevices[i]));
        }
    }

    std::vector<rhi::Adapter*> devices;
    VkInstance instance;
};

rhi::Instance *rhi::getInstance() {
    const VkInstanceCreateInfo kCreateInfo {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &kAppInfo,
        
        .enabledLayerCount = uint32_t(kRequiredLayers.size()),
        .ppEnabledLayerNames = kRequiredLayers.data(),

        .enabledExtensionCount = uint32_t(kRequiredExtensions.size()),
        .ppEnabledExtensionNames = kRequiredExtensions.data()
    };

    VkInstance instance;

    VK_CHECK(vkCreateInstance(&kCreateInfo, nullptr, &instance));

    return new rhiInstance(instance);
}
