#pragma once

// 2D and introductory 3D renderer for StonePotatoes visualization.
// Uses Vulkan presentation when available and OpenGL for CPU-only builds;
// ImGui provides the UI overlay in both configurations.
// 簡單 2D 與基礎 3D 渲染器，用於 StonePotatoes 世界可視化。
// 有 Vulkan 時使用 Vulkan 呈現；CPU-only 建置使用 OpenGL，兩者都以 ImGui 繪製 UI。

#include <glm/glm.hpp>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>

namespace StonePotatoes {
namespace GPU {
class VulkanContext;
}
namespace Rendering {

class Renderer {
public:
    using Texture2DHandle = std::uint32_t;
    static constexpr Texture2DHandle InvalidTexture2D = 0;

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
    // Returns false when the frame could not be submitted/presented.
    bool EndFrame();

    // 2D primitives (world space) | 2D 圖元（世界座標）
    void SetView(float centerX, float centerY, float zoom);
    void DrawPoint(float x, float y, float radius, const glm::vec3& color);
    void DrawCircle(float x, float y, float radius, const glm::vec3& color);

    // RGBA8 texture intended for sprite atlases. Handles are renderer-owned;
    // destroy is deferred until the GPU has released in-flight frame references.
    Texture2DHandle CreateTexture2D(int width, int height,
                                    std::span<const std::uint8_t> rgba8);
    bool DestroyTexture2D(Texture2DHandle texture);
    // World-space sprite center/size, normalized atlas UV rectangle, radians,
    // tint, and stable painter-order layer.
    void DrawSprite2D(Texture2DHandle texture, const glm::vec2& center,
                      const glm::vec2& size,
                      const glm::vec4& uvRect = {0.0f, 0.0f, 1.0f, 1.0f},
                      float rotationRadians = 0.0f,
                      const glm::vec4& tint = {1.0f, 1.0f, 1.0f, 1.0f},
                      int layer = 0);

    // Perspective 3D triangle slice. Opaque triangles are depth-sorted per
    // frame; intersecting surfaces require the future depth-buffer pipeline.
    bool SetCamera3D(const glm::vec3& position, const glm::vec3& target,
                     const glm::vec3& up = {0.0f, 1.0f, 0.0f},
                     float verticalFovRadians = 1.04719755f,
                     float nearPlane = 0.1f, float farPlane = 1000.0f);
    void DrawTriangle3D(const glm::vec3& a, const glm::vec3& b,
                        const glm::vec3& c, const glm::vec3& color);

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
