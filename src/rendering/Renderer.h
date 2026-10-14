#pragma once

// Simple 2D renderer for the StonePotatoes world visualization.
// Uses Vulkan presentation when available and OpenGL for CPU-only builds;
// ImGui provides the UI overlay in both configurations.
// 簡單 2D 渲染器用於 StonePotatoes 世界可視化。
// 有 Vulkan 時使用 Vulkan 呈現；CPU-only 建置使用 OpenGL，兩者都以 ImGui 繪製 UI。

#include <glm/glm.hpp>

#include <memory>

namespace StonePotatoes {
namespace GPU {
class VulkanContext;
}
namespace Rendering {

class Renderer {
public:
    struct Config {
        int width = 1280;
        int height = 720;
        const char* title = "StonePotatoes World";
    };

    Renderer();
    // The supplied context must be uninitialized; Renderer enables GLFW's
    // presentation extensions before selecting the shared compute/graphics device.
    explicit Renderer(std::shared_ptr<GPU::VulkanContext> context);
    ~Renderer();

    bool Initialize(const Config& cfg);
    void Shutdown();

    // Main loop control | 主迴圈控制
    bool ShouldClose() const;
    void BeginFrame();
    void EndFrame();

    // 2D primitives (world space) | 2D 圖元（世界座標）
    void SetView(float centerX, float centerY, float zoom);
    void DrawPoint(float x, float y, float radius, const glm::vec3& color);
    void DrawCircle(float x, float y, float radius, const glm::vec3& color);

    // Immediate-mode ImGui API | ImGui 即時模式 API
    void BeginImGui();
    void EndImGui();

    // Internal GPU context for engine subsystems that share the renderer device.
    // Available after successful initialization; empty in CPU-only builds.
    std::shared_ptr<GPU::VulkanContext> GetVulkanContext() const;

private:
    struct Impl;
    std::unique_ptr<Impl> m_Impl;
};

} // namespace Rendering
} // namespace StonePotatoes
