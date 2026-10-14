#pragma once

#include <memory>
#include <string>

namespace StonePotatoes::GPU {
class VulkanContext;
}

namespace StonePotatoes::Rendering {

// Internal bring-up path for Vulkan presentation. It owns a GLFW window and
// surface, borrows a shared VulkanContext, and presents clear-only frames.
class VulkanPresentationSmoke final {
public:
    struct Config {
        int width = 640;
        int height = 480;
        const char* title = "StonePotatoes Vulkan presentation smoke";
    };

    explicit VulkanPresentationSmoke(
        std::shared_ptr<GPU::VulkanContext> context = {});
    ~VulkanPresentationSmoke();
    VulkanPresentationSmoke(const VulkanPresentationSmoke&) = delete;
    VulkanPresentationSmoke& operator=(const VulkanPresentationSmoke&) = delete;

    bool Initialize();
    bool Initialize(const Config& config);
    bool PresentClearFrame();
    bool ShouldClose() const;
    void PollEvents();
    void Shutdown();

    std::shared_ptr<GPU::VulkanContext> GetContext() const;
    const std::string& LastError() const;

private:
    struct Impl;
    std::unique_ptr<Impl> m_Impl;
};

} // namespace StonePotatoes::Rendering
