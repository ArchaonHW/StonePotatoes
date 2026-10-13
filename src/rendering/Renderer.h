#pragma once

// Simple 2D renderer for the StonePotatoes world visualization.
// Uses OpenGL 3.3 core profile + ImGui for UI overlay.
// 簡單 2D 渲染器用於 StonePotatoes 世界可視化。
// 使用 OpenGL 3.3 core profile + ImGui UI 覆蓋層。

#include <glm/glm.hpp>

#include <memory>

namespace StonePotatoes {
namespace Rendering {

class Renderer {
public:
    struct Config {
        int width = 1280;
        int height = 720;
        const char* title = "StonePotatoes World";
    };

    Renderer();
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

private:
    struct Impl;
    std::unique_ptr<Impl> m_Impl;
};

} // namespace Rendering
} // namespace StonePotatoes
