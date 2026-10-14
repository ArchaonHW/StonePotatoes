#include "VulkanPresentationSmoke.h"

#include "../gpu/VulkanContext.h"

#ifdef SP_VULKAN

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <algorithm>
#include <array>
#include <limits>
#include <utility>
#include <vector>

namespace StonePotatoes::Rendering {

struct VulkanPresentationSmoke::Impl {
    std::shared_ptr<GPU::VulkanContext> context;
    GLFWwindow* window = nullptr;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    VkFormat format = VK_FORMAT_UNDEFINED;
    VkExtent2D extent{};
    std::vector<VkImage> images;
    std::vector<VkImageView> views;
    VkRenderPass renderPass = VK_NULL_HANDLE;
    std::vector<VkFramebuffer> framebuffers;
    VkCommandPool commandPool = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> commandBuffers;
    VkSemaphore imageAvailable = VK_NULL_HANDLE;
    VkSemaphore renderFinished = VK_NULL_HANDLE;
    VkFence frameFence = VK_NULL_HANDLE;
    std::string lastError;
    bool glfwInitialized = false;
    bool swapchainDirty = false;

    bool Fail(const char* message) {
        lastError = message;
        return false;
    }

    bool CreateSwapchain() {
        VkPhysicalDevice physical = context->GetPhysicalDevice();
        VkDevice device = context->GetDevice();
        VkSurfaceCapabilitiesKHR caps{};
        if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
                physical, surface, &caps) != VK_SUCCESS)
            return Fail("failed to query Vulkan surface capabilities");
        if ((caps.supportedUsageFlags & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) == 0)
            return Fail("surface images do not support color attachments");

        uint32_t formatCount = 0;
        if (vkGetPhysicalDeviceSurfaceFormatsKHR(
                physical, surface, &formatCount, nullptr) != VK_SUCCESS ||
            formatCount == 0)
            return Fail("surface has no supported formats");
        std::vector<VkSurfaceFormatKHR> formats(formatCount);
        if (vkGetPhysicalDeviceSurfaceFormatsKHR(
                physical, surface, &formatCount, formats.data()) != VK_SUCCESS)
            return Fail("failed to enumerate surface formats");
        formats.resize(formatCount);
        if (formats.empty()) return Fail("surface has no supported formats");
        VkSurfaceFormatKHR selectedFormat = formats.front();
        if (formats.size() == 1 && selectedFormat.format == VK_FORMAT_UNDEFINED) {
            selectedFormat.format = VK_FORMAT_B8G8R8A8_UNORM;
        } else {
            for (const auto& candidate : formats) {
                if (candidate.format == VK_FORMAT_B8G8R8A8_UNORM ||
                    candidate.format == VK_FORMAT_B8G8R8A8_SRGB) {
                    selectedFormat = candidate;
                    break;
                }
            }
        }

        if (caps.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
            extent = caps.currentExtent;
        } else {
            int width = 0, height = 0;
            glfwGetFramebufferSize(window, &width, &height);
            if (width <= 0 || height <= 0)
                return Fail("window framebuffer has zero extent");
            extent.width = std::clamp(static_cast<uint32_t>(width),
                caps.minImageExtent.width, caps.maxImageExtent.width);
            extent.height = std::clamp(static_cast<uint32_t>(height),
                caps.minImageExtent.height, caps.maxImageExtent.height);
        }

        uint32_t imageCount = caps.minImageCount + 1;
        if (caps.maxImageCount != 0 && imageCount > caps.maxImageCount)
            imageCount = caps.maxImageCount;
        const uint32_t graphicsFamily = context->GetGraphicsQueueFamily();
        const uint32_t presentFamily = context->GetPresentQueueFamily();
        const std::array<uint32_t, 2> families{graphicsFamily, presentFamily};
        VkSwapchainCreateInfoKHR info{VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
        info.surface = surface;
        info.minImageCount = imageCount;
        info.imageFormat = selectedFormat.format;
        info.imageColorSpace = selectedFormat.colorSpace;
        info.imageExtent = extent;
        info.imageArrayLayers = 1;
        info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        if (graphicsFamily != presentFamily) {
            info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            info.queueFamilyIndexCount = static_cast<uint32_t>(families.size());
            info.pQueueFamilyIndices = families.data();
        } else {
            info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }
        info.preTransform = caps.currentTransform;
        const std::array<VkCompositeAlphaFlagBitsKHR, 4> alphaModes{
            VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
            VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
            VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
            VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR};
        for (auto mode : alphaModes) {
            if (caps.supportedCompositeAlpha & mode) {
                info.compositeAlpha = mode;
                break;
            }
        }
        info.presentMode = VK_PRESENT_MODE_FIFO_KHR;
        info.clipped = VK_TRUE;
        if (vkCreateSwapchainKHR(device, &info, nullptr, &swapchain) != VK_SUCCESS)
            return Fail("vkCreateSwapchainKHR failed");
        format = selectedFormat.format;

        if (vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr) !=
            VK_SUCCESS)
            return Fail("failed to query swapchain images");
        images.resize(imageCount);
        if (vkGetSwapchainImagesKHR(device, swapchain, &imageCount,
                                    images.data()) != VK_SUCCESS)
            return Fail("failed to enumerate swapchain images");
        images.resize(imageCount);
        views.reserve(images.size());
        for (VkImage image : images) {
            VkImageViewCreateInfo viewInfo{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
            viewInfo.image = image;
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format = format;
            viewInfo.components = {VK_COMPONENT_SWIZZLE_IDENTITY,
                VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY,
                VK_COMPONENT_SWIZZLE_IDENTITY};
            viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            viewInfo.subresourceRange.levelCount = 1;
            viewInfo.subresourceRange.layerCount = 1;
            VkImageView view = VK_NULL_HANDLE;
            if (vkCreateImageView(device, &viewInfo, nullptr, &view) != VK_SUCCESS)
                return Fail("vkCreateImageView failed");
            views.push_back(view);
        }

        VkAttachmentDescription attachment{};
        attachment.format = format;
        attachment.samples = VK_SAMPLE_COUNT_1_BIT;
        attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        VkAttachmentReference color{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &color;
        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        VkRenderPassCreateInfo renderPassInfo{
            VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
        renderPassInfo.attachmentCount = 1;
        renderPassInfo.pAttachments = &attachment;
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;
        if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) !=
            VK_SUCCESS)
            return Fail("vkCreateRenderPass failed");

        framebuffers.reserve(views.size());
        for (VkImageView view : views) {
            VkFramebufferCreateInfo framebufferInfo{
                VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
            framebufferInfo.renderPass = renderPass;
            framebufferInfo.attachmentCount = 1;
            framebufferInfo.pAttachments = &view;
            framebufferInfo.width = extent.width;
            framebufferInfo.height = extent.height;
            framebufferInfo.layers = 1;
            VkFramebuffer framebuffer = VK_NULL_HANDLE;
            if (vkCreateFramebuffer(device, &framebufferInfo, nullptr,
                                   &framebuffer) != VK_SUCCESS)
                return Fail("vkCreateFramebuffer failed");
            framebuffers.push_back(framebuffer);
        }

        if (commandPool == VK_NULL_HANDLE) {
            VkCommandPoolCreateInfo poolInfo{
                VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
            poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
            poolInfo.queueFamilyIndex = graphicsFamily;
            if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) !=
                VK_SUCCESS)
                return Fail("vkCreateCommandPool failed");
        }
        if (!commandBuffers.empty()) {
            vkFreeCommandBuffers(device, commandPool,
                static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());
            commandBuffers.clear();
        }
        commandBuffers.resize(images.size());
        VkCommandBufferAllocateInfo allocateInfo{
            VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
        allocateInfo.commandPool = commandPool;
        allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocateInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());
        if (vkAllocateCommandBuffers(device, &allocateInfo,
                                     commandBuffers.data()) != VK_SUCCESS)
            return Fail("vkAllocateCommandBuffers failed");
        for (size_t i = 0; i < commandBuffers.size(); ++i) {
            VkCommandBufferBeginInfo beginInfo{
                VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
            if (vkBeginCommandBuffer(commandBuffers[i], &beginInfo) != VK_SUCCESS)
                return Fail("vkBeginCommandBuffer failed");
            VkClearValue clear{};
            clear.color = {{0.08f, 0.12f, 0.18f, 1.0f}};
            VkRenderPassBeginInfo passInfo{
                VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
            passInfo.renderPass = renderPass;
            passInfo.framebuffer = framebuffers[i];
            passInfo.renderArea.extent = extent;
            passInfo.clearValueCount = 1;
            passInfo.pClearValues = &clear;
            vkCmdBeginRenderPass(commandBuffers[i], &passInfo,
                                 VK_SUBPASS_CONTENTS_INLINE);
            vkCmdEndRenderPass(commandBuffers[i]);
            if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS)
                return Fail("vkEndCommandBuffer failed");
        }
        swapchainDirty = false;
        return true;
    }

    void DestroySwapchain() {
        if (!context || !context->GetDevice()) return;
        VkDevice device = context->GetDevice();
        if (!commandBuffers.empty() && commandPool) {
            vkFreeCommandBuffers(device, commandPool,
                static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());
            commandBuffers.clear();
        }
        for (auto framebuffer : framebuffers)
            vkDestroyFramebuffer(device, framebuffer, nullptr);
        framebuffers.clear();
        if (renderPass) vkDestroyRenderPass(device, renderPass, nullptr);
        renderPass = VK_NULL_HANDLE;
        for (auto view : views) vkDestroyImageView(device, view, nullptr);
        views.clear();
        images.clear();
        if (swapchain) vkDestroySwapchainKHR(device, swapchain, nullptr);
        swapchain = VK_NULL_HANDLE;
        format = VK_FORMAT_UNDEFINED;
        extent = {};
    }
};

VulkanPresentationSmoke::VulkanPresentationSmoke(
    std::shared_ptr<GPU::VulkanContext> context)
    : m_Impl(std::make_unique<Impl>()) {
    m_Impl->context = std::move(context);
}

VulkanPresentationSmoke::~VulkanPresentationSmoke() { Shutdown(); }

bool VulkanPresentationSmoke::Initialize(const Config& config) {
    Impl& im = *m_Impl;
    im.lastError.clear();
    if (im.window) return true;
    if (!glfwInit()) return im.Fail("glfwInit failed");
    im.glfwInitialized = true;
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    im.window = glfwCreateWindow(config.width, config.height, config.title,
                                 nullptr, nullptr);
    if (!im.window) {
        Shutdown();
        return im.Fail("failed to create Vulkan GLFW window");
    }

    uint32_t extensionCount = 0;
    const char** extensions = glfwGetRequiredInstanceExtensions(&extensionCount);
    if (!extensions || extensionCount == 0) {
        Shutdown();
        return im.Fail("GLFW returned no Vulkan instance extensions");
    }
    if (!im.context) im.context = std::make_shared<GPU::VulkanContext>();
    std::vector<const char*> requiredExtensions(extensions,
                                                extensions + extensionCount);
    if (!im.context->CreateInstance(requiredExtensions)) {
        const std::string error = im.context->LastError();
        Shutdown();
        return im.Fail(error.c_str());
    }
    if (glfwCreateWindowSurface(im.context->GetInstance(), im.window, nullptr,
                                &im.surface) != VK_SUCCESS) {
        Shutdown();
        return im.Fail("glfwCreateWindowSurface failed");
    }
    if (!im.context->InitializeDevice(0, im.surface)) {
        const std::string error = im.context->LastError();
        Shutdown();
        return im.Fail(error.c_str());
    }
    if (!im.CreateSwapchain()) {
        const std::string error = im.lastError;
        Shutdown();
        return im.Fail(error.c_str());
    }
    VkSemaphoreCreateInfo semaphoreInfo{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    if (vkCreateSemaphore(im.context->GetDevice(), &semaphoreInfo, nullptr,
                          &im.imageAvailable) != VK_SUCCESS ||
        vkCreateSemaphore(im.context->GetDevice(), &semaphoreInfo, nullptr,
                          &im.renderFinished) != VK_SUCCESS) {
        Shutdown();
        return im.Fail("failed to create presentation semaphores");
    }
    VkFenceCreateInfo fenceInfo{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    if (vkCreateFence(im.context->GetDevice(), &fenceInfo, nullptr,
                      &im.frameFence) != VK_SUCCESS) {
        Shutdown();
        return im.Fail("failed to create presentation fence");
    }
    return true;
}

bool VulkanPresentationSmoke::PresentClearFrame() {
    Impl& im = *m_Impl;
    if (!im.window || !im.context || !im.context->IsInitialized())
        return im.Fail("Vulkan presentation is not initialized");
    VkDevice device = im.context->GetDevice();
    int width = 0, height = 0;
    glfwGetFramebufferSize(im.window, &width, &height);
    if (width <= 0 || height <= 0) return true;
    if (im.swapchainDirty ||
        static_cast<uint32_t>(width) != im.extent.width ||
        static_cast<uint32_t>(height) != im.extent.height) {
        if (vkDeviceWaitIdle(device) != VK_SUCCESS)
            return im.Fail("vkDeviceWaitIdle failed before swapchain recreation");
        im.DestroySwapchain();
        if (!im.CreateSwapchain()) return false;
    }
    if (vkWaitForFences(device, 1, &im.frameFence, VK_TRUE, UINT64_MAX) !=
        VK_SUCCESS)
        return im.Fail("vkWaitForFences failed");
    uint32_t imageIndex = 0;
    VkResult acquire = vkAcquireNextImageKHR(device, im.swapchain, UINT64_MAX,
        im.imageAvailable, VK_NULL_HANDLE, &imageIndex);
    if (acquire == VK_ERROR_OUT_OF_DATE_KHR) {
        im.swapchainDirty = true;
        return true;
    }
    if (acquire != VK_SUCCESS && acquire != VK_SUBOPTIMAL_KHR)
        return im.Fail("vkAcquireNextImageKHR failed");
    if (vkResetFences(device, 1, &im.frameFence) != VK_SUCCESS)
        return im.Fail("vkResetFences failed");
    VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submit.waitSemaphoreCount = 1;
    submit.pWaitSemaphores = &im.imageAvailable;
    submit.pWaitDstStageMask = &waitStage;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &im.commandBuffers[imageIndex];
    submit.signalSemaphoreCount = 1;
    submit.pSignalSemaphores = &im.renderFinished;
    if (vkQueueSubmit(im.context->GetGraphicsQueue(), 1, &submit,
                      im.frameFence) != VK_SUCCESS)
        return im.Fail("vkQueueSubmit failed");
    VkPresentInfoKHR present{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
    present.waitSemaphoreCount = 1;
    present.pWaitSemaphores = &im.renderFinished;
    present.swapchainCount = 1;
    present.pSwapchains = &im.swapchain;
    present.pImageIndices = &imageIndex;
    const VkResult result = vkQueuePresentKHR(im.context->GetPresentQueue(),
                                              &present);
    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR &&
        result != VK_ERROR_OUT_OF_DATE_KHR)
        return im.Fail("vkQueuePresentKHR failed");
    if (vkQueueWaitIdle(im.context->GetPresentQueue()) != VK_SUCCESS)
        return im.Fail("vkQueueWaitIdle failed after presentation");
    if (result == VK_SUBOPTIMAL_KHR || result == VK_ERROR_OUT_OF_DATE_KHR ||
        acquire == VK_SUBOPTIMAL_KHR)
        im.swapchainDirty = true;
    return true;
}

bool VulkanPresentationSmoke::ShouldClose() const {
    return !m_Impl->window || glfwWindowShouldClose(m_Impl->window) != 0;
}
void VulkanPresentationSmoke::PollEvents() { glfwPollEvents(); }
std::shared_ptr<GPU::VulkanContext> VulkanPresentationSmoke::GetContext() const {
    return m_Impl->context;
}
const std::string& VulkanPresentationSmoke::LastError() const {
    return m_Impl->lastError;
}

void VulkanPresentationSmoke::Shutdown() {
    Impl& im = *m_Impl;
    if (im.context && im.context->GetDevice())
        vkDeviceWaitIdle(im.context->GetDevice());
    if (im.context && im.context->GetDevice()) {
        VkDevice device = im.context->GetDevice();
        if (im.frameFence) vkDestroyFence(device, im.frameFence, nullptr);
        im.frameFence = VK_NULL_HANDLE;
        if (im.renderFinished)
            vkDestroySemaphore(device, im.renderFinished, nullptr);
        im.renderFinished = VK_NULL_HANDLE;
        if (im.imageAvailable)
            vkDestroySemaphore(device, im.imageAvailable, nullptr);
        im.imageAvailable = VK_NULL_HANDLE;
        im.DestroySwapchain();
        if (im.commandPool) vkDestroyCommandPool(device, im.commandPool, nullptr);
        im.commandPool = VK_NULL_HANDLE;
    }
    if (im.surface && im.context && im.context->GetInstance())
        vkDestroySurfaceKHR(im.context->GetInstance(), im.surface, nullptr);
    im.surface = VK_NULL_HANDLE;
    if (im.window) glfwDestroyWindow(im.window);
    im.window = nullptr;
    if (im.glfwInitialized) glfwTerminate();
    im.glfwInitialized = false;
}

} // namespace StonePotatoes::Rendering

#else

namespace StonePotatoes::Rendering {

struct VulkanPresentationSmoke::Impl {
    std::shared_ptr<GPU::VulkanContext> context;
    std::string lastError = "StonePotatoes was built without Vulkan support";
};

VulkanPresentationSmoke::VulkanPresentationSmoke(
    std::shared_ptr<GPU::VulkanContext> context)
    : m_Impl(std::make_unique<Impl>()) { m_Impl->context = std::move(context); }
VulkanPresentationSmoke::~VulkanPresentationSmoke() = default;
bool VulkanPresentationSmoke::Initialize(const Config&) { return false; }
bool VulkanPresentationSmoke::PresentClearFrame() { return false; }
bool VulkanPresentationSmoke::ShouldClose() const { return true; }
void VulkanPresentationSmoke::PollEvents() {}
void VulkanPresentationSmoke::Shutdown() {}
std::shared_ptr<GPU::VulkanContext> VulkanPresentationSmoke::GetContext() const {
    return m_Impl->context;
}
const std::string& VulkanPresentationSmoke::LastError() const {
    return m_Impl->lastError;
}

} // namespace StonePotatoes::Rendering

#endif
