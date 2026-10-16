#include "Renderer2D.h"

#include <imgui.h>
#include <imgui_internal.h>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>

namespace StonePotatoes::Rendering::Detail {
namespace {

bool IsFinite(const glm::vec2& value) {
    return std::isfinite(value.x) && std::isfinite(value.y);
}

bool IsFinite(const glm::vec4& value) {
    return std::isfinite(value.x) && std::isfinite(value.y) &&
           std::isfinite(value.z) && std::isfinite(value.w);
}

} // namespace

Renderer2D::~Renderer2D() { ShutdownTextures(); }

SpriteTextureHandle Renderer2D::CreateTexture(
    int width, int height, const std::uint8_t* rgba8,
    std::size_t byteCount) {
    constexpr std::size_t kBytesPerPixel = 4;
    constexpr int kMaximumDimension = 16384;
    if (!ImGui::GetCurrentContext() || width <= 0 || height <= 0 ||
        width > kMaximumDimension || height > kMaximumDimension || !rgba8 ||
        static_cast<std::size_t>(width) >
            std::numeric_limits<std::size_t>::max() /
                static_cast<std::size_t>(height) / kBytesPerPixel) {
        return 0;
    }
    const std::size_t requiredBytes = static_cast<std::size_t>(width) *
                                      static_cast<std::size_t>(height) *
                                      kBytesPerPixel;
    if (byteCount != requiredBytes ||
        m_Textures.size() >= std::numeric_limits<SpriteTextureHandle>::max()) {
        return 0;
    }

    ImTextureData* texture = IM_NEW(ImTextureData)();
    texture->Create(ImTextureFormat_RGBA32, width, height);
    std::memcpy(texture->GetPixels(), rgba8, requiredBytes);
    ImGui::RegisterUserTexture(texture);
    m_Textures.push_back(texture);
    m_UserTextureReferencesReleased = false;
    return static_cast<SpriteTextureHandle>(m_Textures.size());
}

bool Renderer2D::DestroyTexture(SpriteTextureHandle handle) {
    if (handle == 0 || handle > m_Textures.size()) return false;
    ImTextureData* texture = m_Textures[handle - 1];
    if (!texture || texture->WantDestroyNextFrame) return false;
    texture->WantDestroyNextFrame = true;
    return true;
}

void Renderer2D::CollectDestroyedTextures() {
    if (!ImGui::GetCurrentContext()) return;
    for (ImTextureData*& texture : m_Textures) {
        if (!texture || !texture->WantDestroyNextFrame ||
            texture->Status != ImTextureStatus_Destroyed) {
            continue;
        }
        ImGui::UnregisterUserTexture(texture);
        IM_DELETE(texture);
        texture = nullptr;
    }
}

void Renderer2D::ShutdownTextures() {
    if (m_Textures.empty()) return;
    const bool hasContext = ImGui::GetCurrentContext() != nullptr;
    for (ImTextureData*& texture : m_Textures) {
        if (!texture) continue;
        if (hasContext && !m_UserTextureReferencesReleased &&
            texture->RefCount > 0)
            ImGui::UnregisterUserTexture(texture);
        IM_DELETE(texture);
        texture = nullptr;
    }
    m_Textures.clear();
    m_Sprites.clear();
    m_UserTextureReferencesReleased = false;
}

void Renderer2D::ReleaseUserTextureReferences() {
    if (m_UserTextureReferencesReleased ||
        !ImGui::GetCurrentContext()) {
        return;
    }
    for (ImTextureData* texture : m_Textures) {
        if (texture && texture->RefCount > 0)
            ImGui::UnregisterUserTexture(texture);
    }
    m_UserTextureReferencesReleased = true;
}

void Renderer2D::SetView(float centerX, float centerY, float zoom) {
    if (!std::isfinite(centerX) || !std::isfinite(centerY) ||
        !std::isfinite(zoom) || zoom <= 0.0f) {
        return;
    }
    m_ViewCenterX = centerX;
    m_ViewCenterY = centerY;
    m_Zoom = zoom;
}

bool Renderer2D::QueueSprite(SpriteTextureHandle texture,
                             const glm::vec2& center,
                             const glm::vec2& size,
                             const glm::vec4& uvRect,
                             float rotationRadians, const glm::vec4& tint,
                             int layer) {
    if (texture == 0 || texture > m_Textures.size() ||
        !m_Textures[texture - 1] ||
        m_Textures[texture - 1]->WantDestroyNextFrame || !IsFinite(center) ||
        !IsFinite(size) || size.x <= 0.0f || size.y <= 0.0f ||
        !IsFinite(uvRect) || uvRect.x < 0.0f || uvRect.y < 0.0f ||
        uvRect.z > 1.0f || uvRect.w > 1.0f || uvRect.x >= uvRect.z ||
        uvRect.y >= uvRect.w || !std::isfinite(rotationRadians) ||
        !IsFinite(tint)) {
        return false;
    }
    m_Sprites.push_back({texture, center, size, uvRect, rotationRadians,
                         glm::clamp(tint, glm::vec4(0.0f), glm::vec4(1.0f)),
                         layer});
    return true;
}

std::vector<ProjectedSprite2D> Renderer2D::ProjectSprites(
    const glm::vec2& viewport) const {
    std::vector<ProjectedSprite2D> projected;
    if (!IsFinite(viewport) || viewport.x <= 0.0f || viewport.y <= 0.0f ||
        !std::isfinite(m_Zoom) || m_Zoom <= 0.0f) {
        return projected;
    }
    projected.reserve(m_Sprites.size());
    const float aspect = viewport.x / viewport.y;
    for (const SpriteCommand& sprite : m_Sprites) {
        const float cosine = std::cos(sprite.rotationRadians);
        const float sine = std::sin(sprite.rotationRadians);
        const std::array<glm::vec2, 4> local{{
            {-sprite.size.x * 0.5f, sprite.size.y * 0.5f},
            {sprite.size.x * 0.5f, sprite.size.y * 0.5f},
            {sprite.size.x * 0.5f, -sprite.size.y * 0.5f},
            {-sprite.size.x * 0.5f, -sprite.size.y * 0.5f},
        }};
        ProjectedSprite2D result;
        result.uvs = {{{sprite.uvRect.x, sprite.uvRect.y},
                       {sprite.uvRect.z, sprite.uvRect.y},
                       {sprite.uvRect.z, sprite.uvRect.w},
                       {sprite.uvRect.x, sprite.uvRect.w}}};
        result.tint = sprite.tint;
        result.texture = sprite.texture;
        result.layer = sprite.layer;
        for (std::size_t i = 0; i < local.size(); ++i) {
            const glm::vec2 rotated(
                local[i].x * cosine - local[i].y * sine,
                local[i].x * sine + local[i].y * cosine);
            const glm::vec2 world = sprite.center + rotated;
            result.positions[i] = {
                (world.x - m_ViewCenterX) * m_Zoom / aspect *
                        viewport.x * 0.5f +
                    viewport.x * 0.5f,
                viewport.y * 0.5f -
                    (world.y - m_ViewCenterY) * m_Zoom * viewport.y * 0.5f};
            if (!IsFinite(result.positions[i])) break;
        }
        if (!IsFinite(result.positions[0]) ||
            !IsFinite(result.positions[1]) ||
            !IsFinite(result.positions[2]) ||
            !IsFinite(result.positions[3])) {
            continue;
        }
        projected.push_back(result);
    }

    std::stable_sort(projected.begin(), projected.end(),
                     [](const ProjectedSprite2D& a,
                        const ProjectedSprite2D& b) {
                         return a.layer < b.layer;
                     });
    return projected;
}

void Renderer2D::Flush(ImDrawList* drawList, const glm::vec2& viewport) {
    if (drawList) {
        for (const ProjectedSprite2D& sprite : ProjectSprites(viewport)) {
            if (sprite.texture == 0 || sprite.texture > m_Textures.size())
                continue;
            ImTextureData* texture = m_Textures[sprite.texture - 1];
            if (!texture || texture->WantDestroyNextFrame) continue;
            const ImU32 tint = ImGui::ColorConvertFloat4ToU32(
                ImVec4(sprite.tint.r, sprite.tint.g, sprite.tint.b,
                       sprite.tint.a));
            drawList->AddImageQuad(
                texture->GetTexRef(),
                ImVec2(sprite.positions[0].x, sprite.positions[0].y),
                ImVec2(sprite.positions[1].x, sprite.positions[1].y),
                ImVec2(sprite.positions[2].x, sprite.positions[2].y),
                ImVec2(sprite.positions[3].x, sprite.positions[3].y),
                ImVec2(sprite.uvs[0].x, sprite.uvs[0].y),
                ImVec2(sprite.uvs[1].x, sprite.uvs[1].y),
                ImVec2(sprite.uvs[2].x, sprite.uvs[2].y),
                ImVec2(sprite.uvs[3].x, sprite.uvs[3].y),
                tint);
        }
    }
    ClearSprites();
}

void Renderer2D::ClearSprites() { m_Sprites.clear(); }

} // namespace StonePotatoes::Rendering::Detail
