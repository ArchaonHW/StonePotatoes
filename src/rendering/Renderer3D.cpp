#include "Renderer3D.h"

#include <imgui.h>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

#include <algorithm>
#include <cmath>

namespace StonePotatoes::Rendering::Detail {
namespace {

bool IsFinite(const glm::vec3& value) {
    return std::isfinite(value.x) && std::isfinite(value.y) &&
           std::isfinite(value.z);
}

bool IsFinite(const glm::vec2& value) {
    return std::isfinite(value.x) && std::isfinite(value.y);
}

bool Project(const glm::mat4& viewProjection, const glm::vec3& position,
             const glm::vec2& viewport, glm::vec2& screen) {
    const glm::vec4 clip = viewProjection * glm::vec4(position, 1.0f);
    if (!std::isfinite(clip.x) || !std::isfinite(clip.y) ||
        !std::isfinite(clip.w) || clip.w <= 0.0f) {
        return false;
    }
    const float x = clip.x / clip.w;
    const float y = clip.y / clip.w;
    if (!std::isfinite(x) || !std::isfinite(y)) return false;
    screen = {(x * 0.5f + 0.5f) * viewport.x,
              (0.5f - y * 0.5f) * viewport.y};
    return IsFinite(screen);
}

} // namespace

bool Renderer3D::SetCamera(const glm::vec3& position,
                           const glm::vec3& target, const glm::vec3& up,
                           float verticalFovRadians, float nearPlane,
                           float farPlane) {
    constexpr float kMinimumFov = 0.0174532925f;
    constexpr float kMaximumFov = 3.12413936f;
    const glm::vec3 direction = target - position;
    const glm::vec3 side = glm::cross(direction, up);
    const float directionLengthSquared = glm::dot(direction, direction);
    const float sideLengthSquared = glm::dot(side, side);
    if (!IsFinite(position) || !IsFinite(target) || !IsFinite(up) ||
        !IsFinite(direction) || !IsFinite(side) ||
        !std::isfinite(directionLengthSquared) ||
        !std::isfinite(sideLengthSquared) ||
        directionLengthSquared <= 1.0e-12f ||
        sideLengthSquared <= 1.0e-12f ||
        !std::isfinite(verticalFovRadians) ||
        verticalFovRadians < kMinimumFov ||
        verticalFovRadians > kMaximumFov || !std::isfinite(nearPlane) ||
        !std::isfinite(farPlane) || nearPlane <= 0.0f ||
        farPlane <= nearPlane) {
        return false;
    }

    m_Position = position;
    m_Target = target;
    m_Up = up;
    m_VerticalFovRadians = verticalFovRadians;
    m_NearPlane = nearPlane;
    m_FarPlane = farPlane;
    return true;
}

void Renderer3D::DrawTriangle(const glm::vec3& a, const glm::vec3& b,
                              const glm::vec3& c, const glm::vec3& color) {
    if (!IsFinite(a) || !IsFinite(b) || !IsFinite(c) || !IsFinite(color))
        return;
    m_Triangles.push_back({a, b, c,
                           glm::clamp(color, glm::vec3(0.0f),
                                      glm::vec3(1.0f))});
}

std::vector<ProjectedTriangle> Renderer3D::ProjectTriangles(
    const glm::vec2& viewport) const {
    std::vector<ProjectedTriangle> projected;
    if (!IsFinite(viewport) || viewport.x <= 0.0f || viewport.y <= 0.0f)
        return projected;

    const glm::mat4 view = glm::lookAtRH(m_Position, m_Target, m_Up);
    const glm::mat4 projection = glm::perspectiveRH_NO(
        m_VerticalFovRadians, viewport.x / viewport.y, m_NearPlane,
        m_FarPlane);
    const glm::mat4 viewProjection = projection * view;
    projected.reserve(m_Triangles.size());

    for (const Triangle& triangle : m_Triangles) {
        const glm::vec4 viewA = view * glm::vec4(triangle.a, 1.0f);
        const glm::vec4 viewB = view * glm::vec4(triangle.b, 1.0f);
        const glm::vec4 viewC = view * glm::vec4(triangle.c, 1.0f);
        const float depthA = -viewA.z;
        const float depthB = -viewB.z;
        const float depthC = -viewC.z;
        if (!std::isfinite(depthA) || !std::isfinite(depthB) ||
            !std::isfinite(depthC) || depthA < m_NearPlane ||
            depthB < m_NearPlane || depthC < m_NearPlane ||
            depthA > m_FarPlane || depthB > m_FarPlane ||
            depthC > m_FarPlane) {
            continue;
        }

        ProjectedTriangle result;
        if (!Project(viewProjection, triangle.a, viewport, result.a) ||
            !Project(viewProjection, triangle.b, viewport, result.b) ||
            !Project(viewProjection, triangle.c, viewport, result.c)) {
            continue;
        }
        const float area = (result.b.x - result.a.x) *
                               (result.c.y - result.a.y) -
                           (result.b.y - result.a.y) *
                               (result.c.x - result.a.x);
        if (!std::isfinite(area) || std::abs(area) <= 1.0e-4f) continue;
        result.color = triangle.color;
        result.depth = (depthA + depthB + depthC) / 3.0f;
        projected.push_back(result);
    }

    std::stable_sort(projected.begin(), projected.end(),
                     [](const ProjectedTriangle& a,
                        const ProjectedTriangle& b) {
                         return a.depth > b.depth;
                     });
    return projected;
}

void Renderer3D::Flush(ImDrawList* drawList, const glm::vec2& viewport) {
    if (drawList) {
        for (const ProjectedTriangle& triangle : ProjectTriangles(viewport)) {
            const ImU32 color = ImGui::ColorConvertFloat4ToU32(
                ImVec4(triangle.color.r, triangle.color.g, triangle.color.b,
                       1.0f));
            drawList->AddTriangleFilled(
                ImVec2(triangle.a.x, triangle.a.y),
                ImVec2(triangle.b.x, triangle.b.y),
                ImVec2(triangle.c.x, triangle.c.y), color);
        }
    }
    Clear();
}

void Renderer3D::Clear() { m_Triangles.clear(); }

} // namespace StonePotatoes::Rendering::Detail
