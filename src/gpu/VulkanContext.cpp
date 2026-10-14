#include "VulkanContext.h"

#ifdef SP_VULKAN

#include <algorithm>
#include <cstring>
#include <limits>
#include <utility>
#include <vector>

namespace StonePotatoes::GPU {

struct VulkanContext::Impl {
    VkInstance instance = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    uint32_t computeQueueFamily = std::numeric_limits<uint32_t>::max();
    uint32_t graphicsQueueFamily = std::numeric_limits<uint32_t>::max();
    uint32_t presentQueueFamily = std::numeric_limits<uint32_t>::max();
    uint32_t timestampValidBits = 0;
    VkQueue computeQueue = VK_NULL_HANDLE;
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    VkQueue presentQueue = VK_NULL_HANDLE;
    VkSurfaceKHR presentationSurface = VK_NULL_HANDLE; // non-owning
    std::vector<std::string> enabledInstanceExtensions;
    DeviceCaps caps;
    std::string lastError;
};

VulkanContext::VulkanContext() : m_Impl(std::make_unique<Impl>()) {}
VulkanContext::~VulkanContext() { Shutdown(); }

bool VulkanContext::Initialize(uint32_t deviceIndex) {
    return CreateInstance() && InitializeDevice(deviceIndex);
}

bool VulkanContext::CreateInstance(
    const std::vector<const char*>& requiredInstanceExtensions) {
    Impl& im = *m_Impl;
    im.lastError.clear();
    if (im.instance != VK_NULL_HANDLE) {
        for (const char* required : requiredInstanceExtensions) {
            const bool alreadyEnabled = std::find(
                im.enabledInstanceExtensions.begin(),
                im.enabledInstanceExtensions.end(), required) !=
                im.enabledInstanceExtensions.end();
            if (!alreadyEnabled) {
                im.lastError = "Vulkan instance already exists without required extension: ";
                im.lastError += required;
                return false;
            }
        }
        return true;
    }

    uint32_t extensionCount = 0;
    if (vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount,
                                                nullptr) != VK_SUCCESS) {
        im.lastError = "vkEnumerateInstanceExtensionProperties failed";
        return false;
    }
    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    if (extensionCount != 0 &&
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount,
                                                availableExtensions.data()) !=
            VK_SUCCESS) {
        im.lastError = "vkEnumerateInstanceExtensionProperties failed";
        return false;
    }
    for (const char* required : requiredInstanceExtensions) {
        const bool available = std::any_of(
            availableExtensions.begin(), availableExtensions.end(),
            [required](const VkExtensionProperties& extension) {
                return std::strcmp(extension.extensionName, required) == 0;
            });
        if (!available) {
            im.lastError = "required Vulkan instance extension is unavailable: ";
            im.lastError += required;
            return false;
        }
    }

    VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};
    app.pApplicationName = "StonePotatoes";
    app.apiVersion = VK_API_VERSION_1_1;
    VkInstanceCreateInfo createInfo{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    createInfo.pApplicationInfo = &app;
    createInfo.enabledExtensionCount =
        static_cast<uint32_t>(requiredInstanceExtensions.size());
    createInfo.ppEnabledExtensionNames = requiredInstanceExtensions.data();
    if (vkCreateInstance(&createInfo, nullptr, &im.instance) != VK_SUCCESS) {
        im.lastError = "vkCreateInstance failed";
        return false;
    }
    for (const char* extension : requiredInstanceExtensions)
        im.enabledInstanceExtensions.emplace_back(extension);
    return true;
}

bool VulkanContext::InitializeDevice(uint32_t deviceIndex,
                                     VkSurfaceKHR presentationSurface) {
    Impl& im = *m_Impl;
    im.lastError.clear();
    if (im.instance == VK_NULL_HANDLE && !CreateInstance()) return false;
    if (im.device != VK_NULL_HANDLE) {
        if (im.presentationSurface == presentationSurface) return true;
        im.lastError = "Vulkan device already initialized for a different surface";
        return false;
    }
    if (presentationSurface != VK_NULL_HANDLE &&
        std::find(im.enabledInstanceExtensions.begin(),
                  im.enabledInstanceExtensions.end(),
                  VK_KHR_SURFACE_EXTENSION_NAME) ==
            im.enabledInstanceExtensions.end()) {
        im.lastError = "VK_KHR_surface must be enabled before selecting a presentation device";
        return false;
    }

    uint32_t physicalDeviceCount = 0;
    const VkResult enumerateResult = vkEnumeratePhysicalDevices(
        im.instance, &physicalDeviceCount, nullptr);
    if (enumerateResult != VK_SUCCESS || physicalDeviceCount == 0) {
        im.lastError = "no Vulkan physical devices";
        return false;
    }
    std::vector<VkPhysicalDevice> devices(physicalDeviceCount);
    if (vkEnumeratePhysicalDevices(im.instance, &physicalDeviceCount,
                                   devices.data()) != VK_SUCCESS) {
        im.lastError = "vkEnumeratePhysicalDevices failed";
        return false;
    }
    devices.resize(physicalDeviceCount);

    struct Candidate {
        VkPhysicalDevice device = VK_NULL_HANDLE;
        uint32_t computeFamily = UINT32_MAX;
        uint32_t graphicsFamily = UINT32_MAX;
        uint32_t presentFamily = UINT32_MAX;
        VkQueueFamilyProperties computeProperties{};
        VkPhysicalDeviceProperties properties{};
    };
    std::vector<Candidate> candidates;
    for (VkPhysicalDevice device : devices) {
        uint32_t familyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &familyCount, nullptr);
        std::vector<VkQueueFamilyProperties> families(familyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &familyCount,
                                                  families.data());
        Candidate candidate;
        candidate.device = device;
        for (uint32_t i = 0; i < familyCount; ++i) {
            const auto flags = families[i].queueFlags;
            if ((flags & VK_QUEUE_COMPUTE_BIT) &&
                (candidate.computeFamily == UINT32_MAX ||
                 ((families[candidate.computeFamily].queueFlags &
                   VK_QUEUE_GRAPHICS_BIT) &&
                  !(flags & VK_QUEUE_GRAPHICS_BIT)))) {
                candidate.computeFamily = i;
                candidate.computeProperties = families[i];
            }
            if (presentationSurface == VK_NULL_HANDLE) continue;
            if ((flags & VK_QUEUE_GRAPHICS_BIT) &&
                candidate.graphicsFamily == UINT32_MAX)
                candidate.graphicsFamily = i;
            VkBool32 canPresent = VK_FALSE;
            if (vkGetPhysicalDeviceSurfaceSupportKHR(device, i,
                    presentationSurface, &canPresent) == VK_SUCCESS &&
                canPresent && candidate.presentFamily == UINT32_MAX)
                candidate.presentFamily = i;
        }
        if (candidate.computeFamily == UINT32_MAX) continue;
        if (presentationSurface != VK_NULL_HANDLE &&
            (candidate.graphicsFamily == UINT32_MAX ||
             candidate.presentFamily == UINT32_MAX))
            continue;

        if (presentationSurface != VK_NULL_HANDLE) {
            uint32_t deviceExtensionCount = 0;
            if (vkEnumerateDeviceExtensionProperties(device, nullptr,
                    &deviceExtensionCount, nullptr) != VK_SUCCESS)
                continue;
            std::vector<VkExtensionProperties> deviceExtensions(
                deviceExtensionCount);
            if (vkEnumerateDeviceExtensionProperties(device, nullptr,
                    &deviceExtensionCount, deviceExtensions.data()) != VK_SUCCESS)
                continue;
            const bool hasSwapchain = std::any_of(deviceExtensions.begin(),
                deviceExtensions.end(), [](const VkExtensionProperties& ext) {
                    return std::strcmp(ext.extensionName,
                                       VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0;
                });
            if (!hasSwapchain) continue;
        }
        vkGetPhysicalDeviceProperties(device, &candidate.properties);
        candidates.push_back(candidate);
    }
    std::stable_sort(candidates.begin(), candidates.end(),
                     [](const Candidate& a, const Candidate& b) {
        const auto rank = [](VkPhysicalDeviceType type) {
            return type == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ? 0 :
                   type == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU ? 1 : 2;
        };
        return rank(a.properties.deviceType) < rank(b.properties.deviceType);
    });
    if (candidates.empty()) {
        im.lastError = presentationSurface == VK_NULL_HANDLE
            ? "no compute-capable device"
            : "no device supports compute, graphics, presentation, and swapchain";
        return false;
    }
    if (deviceIndex >= candidates.size()) deviceIndex = 0;
    const Candidate& selected = candidates[deviceIndex];

    std::vector<uint32_t> uniqueFamilies{selected.computeFamily};
    if (presentationSurface != VK_NULL_HANDLE) {
        for (uint32_t family : {selected.graphicsFamily, selected.presentFamily})
            if (std::find(uniqueFamilies.begin(), uniqueFamilies.end(), family) ==
                uniqueFamilies.end())
                uniqueFamilies.push_back(family);
    }
    const float priority = 1.0f;
    std::vector<VkDeviceQueueCreateInfo> queueInfos;
    queueInfos.reserve(uniqueFamilies.size());
    for (uint32_t family : uniqueFamilies) {
        VkDeviceQueueCreateInfo queueInfo{
            VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
        queueInfo.queueFamilyIndex = family;
        queueInfo.queueCount = 1;
        queueInfo.pQueuePriorities = &priority;
        queueInfos.push_back(queueInfo);
    }
    VkPhysicalDeviceFeatures availableFeatures{};
    vkGetPhysicalDeviceFeatures(selected.device, &availableFeatures);
    VkPhysicalDeviceFeatures enabledFeatures{};
    enabledFeatures.shaderFloat64 = availableFeatures.shaderFloat64;
    const char* swapchainExtension = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
    VkDeviceCreateInfo deviceInfo{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    deviceInfo.queueCreateInfoCount = static_cast<uint32_t>(queueInfos.size());
    deviceInfo.pQueueCreateInfos = queueInfos.data();
    deviceInfo.pEnabledFeatures = &enabledFeatures;
    if (presentationSurface != VK_NULL_HANDLE) {
        deviceInfo.enabledExtensionCount = 1;
        deviceInfo.ppEnabledExtensionNames = &swapchainExtension;
    }
    if (vkCreateDevice(selected.device, &deviceInfo, nullptr, &im.device) !=
        VK_SUCCESS) {
        im.lastError = "vkCreateDevice failed";
        return false;
    }

    im.physicalDevice = selected.device;
    im.computeQueueFamily = selected.computeFamily;
    im.graphicsQueueFamily = selected.graphicsFamily;
    im.presentQueueFamily = selected.presentFamily;
    im.presentationSurface = presentationSurface;
    vkGetDeviceQueue(im.device, im.computeQueueFamily, 0, &im.computeQueue);
    if (presentationSurface != VK_NULL_HANDLE) {
        vkGetDeviceQueue(im.device, im.graphicsQueueFamily, 0,
                         &im.graphicsQueue);
        vkGetDeviceQueue(im.device, im.presentQueueFamily, 0,
                         &im.presentQueue);
    }

    VkPhysicalDeviceMemoryProperties memory{};
    vkGetPhysicalDeviceMemoryProperties(im.physicalDevice, &memory);
    im.caps.type = selected.properties.deviceType ==
                           VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU
                       ? DeviceType::DiscreteGPU
                       : DeviceType::IntegratedGPU;
    im.caps.name = selected.properties.deviceName;
    im.caps.computeUnits = selected.computeProperties.queueCount;
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
    im.timestampValidBits = selected.computeProperties.timestampValidBits;
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
    im.computeQueueFamily = std::numeric_limits<uint32_t>::max();
    im.graphicsQueueFamily = std::numeric_limits<uint32_t>::max();
    im.presentQueueFamily = std::numeric_limits<uint32_t>::max();
    im.timestampValidBits = 0;
    im.computeQueue = VK_NULL_HANDLE;
    im.graphicsQueue = VK_NULL_HANDLE;
    im.presentQueue = VK_NULL_HANDLE;
    im.presentationSurface = VK_NULL_HANDLE;
    im.enabledInstanceExtensions.clear();
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
VkQueue VulkanContext::GetGraphicsQueue() const {
    return m_Impl->graphicsQueue;
}
VkQueue VulkanContext::GetPresentQueue() const { return m_Impl->presentQueue; }
uint32_t VulkanContext::GetComputeQueueFamily() const {
    return m_Impl->computeQueueFamily;
}
uint32_t VulkanContext::GetGraphicsQueueFamily() const {
    return m_Impl->graphicsQueueFamily;
}
uint32_t VulkanContext::GetPresentQueueFamily() const {
    return m_Impl->presentQueueFamily;
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
