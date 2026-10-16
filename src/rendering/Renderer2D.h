#pragma once

#include <glm/glm.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

struct ImDrawList;
struct ImTextureData;

namespace StonePotatoes::Rendering::Detail {

using SpriteTextureHandle = std::uint32_t;

struct ProjectedSprite2D {
    std::array<glm::vec2, 4> positions{};
    std::array<glm::vec2, 4> uvs{};
    glm::vec4 tint{1.0f};
    SpriteTextureHandle texture = 0;
    int layer = 0;
};

class Renderer2D final {
public:
    ~Renderer2D();
    Renderer2D() = default;
    Renderer2D(const Renderer2D&) = delete;
    Renderer2D& operator=(const Renderer2D&) = delete;

    SpriteTextureHandle CreateTexture(int width, int height,
                                      const std::uint8_t* rgba8,
                                      std::size_t byteCount);
    bool DestroyTexture(SpriteTextureHandle texture);
    void CollectDestroyedTextures();
    void ReleaseUserTextureReferences();
    void ShutdownTextures();

    void SetView(float centerX, float centerY, float zoom);
    bool QueueSprite(SpriteTextureHandle texture, const glm::vec2& center,
                     const glm::vec2& size, const glm::vec4& uvRect,
                     float rotationRadians, const glm::vec4& tint, int layer);
    std::vector<ProjectedSprite2D> ProjectSprites(
        const glm::vec2& viewport) const;
    void Flush(ImDrawList* drawList, const glm::vec2& viewport);
    void ClearSprites();

private:
    struct SpriteCommand {
        SpriteTextureHandle texture = 0;
        glm::vec2 center{};
        glm::vec2 size{};
        glm::vec4 uvRect{};
        float rotationRadians = 0.0f;
        glm::vec4 tint{};
        int layer = 0;
    };

    std::vector<ImTextureData*> m_Textures;
    std::vector<SpriteCommand> m_Sprites;
    float m_ViewCenterX = 0.0f;
    float m_ViewCenterY = 0.0f;
    float m_Zoom = 1.0f;
    bool m_UserTextureReferencesReleased = false;
};

} // namespace StonePotatoes::Rendering::Detail
