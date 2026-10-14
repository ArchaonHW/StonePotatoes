#pragma once

#include "ComputeDevice.h"

#include <cstdint>
#include <memory>
#include <string>

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
    VkInstance GetInstance() const;
    VkPhysicalDevice GetPhysicalDevice() const;
    VkDevice GetDevice() const;
    VkQueue GetComputeQueue() const;
    uint32_t GetComputeQueueFamily() const;
    uint32_t GetTimestampValidBits() const;
#endif

private:
    void Shutdown();
    struct Impl;
    std::unique_ptr<Impl> m_Impl;
};

} // namespace StonePotatoes::GPU
