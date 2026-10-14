#pragma once

#include "ComputeDevice.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#ifdef SP_VULKAN
#include <vulkan/vulkan.h>
#endif

namespace StonePotatoes::GPU {

// Owns Vulkan instance/device/compute-queue lifetime. This is an internal
// engine API so the installed headless package does not expose Vulkan types.
class VulkanContext final {
public:
    VulkanContext();
    ~VulkanContext();
    VulkanContext(const VulkanContext&) = delete;
    VulkanContext& operator=(const VulkanContext&) = delete;

    bool Initialize(uint32_t deviceIndex = 0);
    bool IsInitialized() const;
    const DeviceCaps& GetCaps() const;
    const std::string& LastError() const;

#ifdef SP_VULKAN
    bool CreateInstance(
        const std::vector<const char*>& requiredInstanceExtensions = {});
    // Pass a non-owning surface after instance creation to require presentation
    // support. Graphics/present queue-family getters return UINT32_MAX when
    // the context was initialized headlessly.
    bool InitializeDevice(uint32_t deviceIndex = 0,
                          VkSurfaceKHR presentationSurface = VK_NULL_HANDLE);
    VkInstance GetInstance() const;
    VkPhysicalDevice GetPhysicalDevice() const;
    VkDevice GetDevice() const;
    VkQueue GetComputeQueue() const;
    VkQueue GetGraphicsQueue() const;
    VkQueue GetPresentQueue() const;
    uint32_t GetComputeQueueFamily() const;
    uint32_t GetGraphicsQueueFamily() const;
    uint32_t GetPresentQueueFamily() const;
    uint32_t GetTimestampValidBits() const;
#endif

private:
    void Shutdown();
    struct Impl;
    std::unique_ptr<Impl> m_Impl;
};

} // namespace StonePotatoes::GPU
