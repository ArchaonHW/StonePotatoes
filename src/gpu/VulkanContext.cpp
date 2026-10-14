#include "VulkanContext.h"

#ifdef SP_VULKAN

#include <algorithm>
#include <limits>
#include <utility>
#include <vector>

namespace StonePotatoes::GPU {

struct VulkanContext::Impl {
    VkInstance instance = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    uint32_t computeQueueFamily = 0;
    uint32_t timestampValidBits = 0;
    VkQueue computeQueue = VK_NULL_HANDLE;
    DeviceCaps caps;
    std::string lastError;
};

VulkanContext::VulkanContext() : m_Impl(std::make_unique<Impl>()) {}
VulkanContext::~VulkanContext() { Shutdown(); }

bool VulkanContext::Initialize(uint32_t deviceIndex) {
    Impl& im = *m_Impl;
    if (im.device != VK_NULL_HANDLE) return true;
    im.lastError.clear();

    VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};
    app.pApplicationName = "StonePotatoes";
    app.apiVersion = VK_API_VERSION_1_1;
    VkInstanceCreateInfo ici{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    ici.pApplicationInfo = &app;
    if (vkCreateInstance(&ici, nullptr, &im.instance) != VK_SUCCESS) {
        im.lastError = "vkCreateInstance failed";
        return false;
    }

    uint32_t physicalDeviceCount = 0;
    vkEnumeratePhysicalDevices(im.instance, &physicalDeviceCount, nullptr);
    std::vector<VkPhysicalDevice> devices(physicalDeviceCount);
    if (physicalDeviceCount != 0) {
        vkEnumeratePhysicalDevices(im.instance, &physicalDeviceCount,
                                   devices.data());
        devices.resize(physicalDeviceCount);
    }
    if (devices.empty()) {
        im.lastError = "no Vulkan physical devices";
        Shutdown();
        return false;
    }

    std::vector<VkPhysicalDevice> computeDevices;
    for (VkPhysicalDevice device : devices) {
        uint32_t familyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &familyCount, nullptr);
        std::vector<VkQueueFamilyProperties> families(familyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &familyCount,
                                                  families.data());
        if (std::any_of(families.begin(), families.end(), [](const auto& family) {
                return (family.queueFlags & VK_QUEUE_COMPUTE_BIT) != 0;
            })) {
            computeDevices.push_back(device);
        }
    }

    std::stable_sort(computeDevices.begin(), computeDevices.end(),
                     [](VkPhysicalDevice a, VkPhysicalDevice b) {
        VkPhysicalDeviceProperties aProperties{}, bProperties{};
        vkGetPhysicalDeviceProperties(a, &aProperties);
        vkGetPhysicalDeviceProperties(b, &bProperties);
        const auto rank = [](VkPhysicalDeviceType type) {
            return type == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ? 0 :
                   type == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU ? 1 : 2;
        };
        return rank(aProperties.deviceType) < rank(bProperties.deviceType);
    });
    if (computeDevices.empty()) {
        im.lastError = "no compute-capable device";
        Shutdown();
        return false;
    }
    if (deviceIndex >= computeDevices.size()) deviceIndex = 0;
    im.physicalDevice = computeDevices[deviceIndex];

    uint32_t familyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(im.physicalDevice, &familyCount,
                                              nullptr);
    std::vector<VkQueueFamilyProperties> families(familyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(im.physicalDevice, &familyCount,
                                              families.data());
    im.computeQueueFamily = familyCount;
    uint32_t bestScore = 0;
    for (uint32_t i = 0; i < familyCount; ++i) {
        if ((families[i].queueFlags & VK_QUEUE_COMPUTE_BIT) == 0) continue;
        const uint32_t score =
            (families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) ? 1 : 2;
        if (score > bestScore) {
            bestScore = score;
            im.computeQueueFamily = i;
        }
    }
    if (im.computeQueueFamily == familyCount) {
        im.lastError = "no compute queue family";
        Shutdown();
        return false;
    }

    const float priority = 1.0f;
    VkDeviceQueueCreateInfo queueInfo{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
    queueInfo.queueFamilyIndex = im.computeQueueFamily;
    queueInfo.queueCount = 1;
    queueInfo.pQueuePriorities = &priority;
    VkPhysicalDeviceFeatures availableFeatures{};
    vkGetPhysicalDeviceFeatures(im.physicalDevice, &availableFeatures);
    VkPhysicalDeviceFeatures enabledFeatures{};
    enabledFeatures.shaderFloat64 = availableFeatures.shaderFloat64;
    VkDeviceCreateInfo deviceInfo{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    deviceInfo.queueCreateInfoCount = 1;
    deviceInfo.pQueueCreateInfos = &queueInfo;
    deviceInfo.pEnabledFeatures = &enabledFeatures;
    if (vkCreateDevice(im.physicalDevice, &deviceInfo, nullptr, &im.device) !=
        VK_SUCCESS) {
        im.lastError = "vkCreateDevice failed";
        Shutdown();
        return false;
    }
    vkGetDeviceQueue(im.device, im.computeQueueFamily, 0, &im.computeQueue);

    VkPhysicalDeviceProperties properties{};
    VkPhysicalDeviceMemoryProperties memory{};
    vkGetPhysicalDeviceProperties(im.physicalDevice, &properties);
    vkGetPhysicalDeviceMemoryProperties(im.physicalDevice, &memory);
    im.caps.type = properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU
                       ? DeviceType::DiscreteGPU
                       : DeviceType::IntegratedGPU;
    im.caps.name = properties.deviceName;
    im.caps.computeUnits = families[im.computeQueueFamily].queueCount;
    for (uint32_t i = 0; i < memory.memoryHeapCount; ++i) {
        if (memory.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
            im.caps.memoryBytes += memory.memoryHeaps[i].size;
    }
    if (!im.caps.memoryBytes) {
        for (uint32_t i = 0; i < memory.memoryHeapCount; ++i)
            im.caps.memoryBytes += memory.memoryHeaps[i].size;
    }
    im.caps.supportsFloat64 = availableFeatures.shaderFloat64 == VK_TRUE;
    im.caps.supportsAtomics = true;
    im.timestampValidBits = families[im.computeQueueFamily].timestampValidBits;
    return true;
}

void VulkanContext::Shutdown() {
    Impl& im = *m_Impl;
    if (im.device != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(im.device);
        vkDestroyDevice(im.device, nullptr);
    }
    if (im.instance != VK_NULL_HANDLE) vkDestroyInstance(im.instance, nullptr);
    im.instance = VK_NULL_HANDLE;
    im.physicalDevice = VK_NULL_HANDLE;
    im.device = VK_NULL_HANDLE;
    im.computeQueueFamily = 0;
    im.timestampValidBits = 0;
    im.computeQueue = VK_NULL_HANDLE;
    im.caps = DeviceCaps{};
}

bool VulkanContext::IsInitialized() const {
    return m_Impl->device != VK_NULL_HANDLE;
}
const DeviceCaps& VulkanContext::GetCaps() const { return m_Impl->caps; }
const std::string& VulkanContext::LastError() const { return m_Impl->lastError; }
VkInstance VulkanContext::GetInstance() const { return m_Impl->instance; }
VkPhysicalDevice VulkanContext::GetPhysicalDevice() const {
    return m_Impl->physicalDevice;
}
VkDevice VulkanContext::GetDevice() const { return m_Impl->device; }
VkQueue VulkanContext::GetComputeQueue() const { return m_Impl->computeQueue; }
uint32_t VulkanContext::GetComputeQueueFamily() const {
    return m_Impl->computeQueueFamily;
}
uint32_t VulkanContext::GetTimestampValidBits() const {
    return m_Impl->timestampValidBits;
}

} // namespace StonePotatoes::GPU

#else

namespace StonePotatoes::GPU {

struct VulkanContext::Impl {
    std::string lastError = "built without SP_VULKAN";
    DeviceCaps caps;
};

VulkanContext::VulkanContext() : m_Impl(std::make_unique<Impl>()) {}
VulkanContext::~VulkanContext() = default;
bool VulkanContext::Initialize(uint32_t) { return false; }
void VulkanContext::Shutdown() {}
bool VulkanContext::IsInitialized() const { return false; }
const DeviceCaps& VulkanContext::GetCaps() const { return m_Impl->caps; }
const std::string& VulkanContext::LastError() const { return m_Impl->lastError; }

} // namespace StonePotatoes::GPU

#endif
