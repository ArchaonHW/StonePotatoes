#include "Renderer3D.h"
#include "PBRLighting.h"

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

bool Project(const glm::mat4& projection, const glm::vec3& viewPosition,
             const glm::vec2& viewport, glm::vec2& screen) {
    const glm::vec4 clip = projection * glm::vec4(viewPosition, 1.0f);
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

std::vector<glm::vec3> ClipDepthPlane(const std::vector<glm::vec3>& input,
                                      float planeDepth, bool keepGreater) {
    std::vector<glm::vec3> output;
    if (input.empty()) return output;
    output.reserve(input.size() + 1);

    const auto inside = [planeDepth, keepGreater](float depth) {
        return keepGreater ? depth >= planeDepth : depth <= planeDepth;
    };
    glm::vec3 previous = input.back();
    float previousDepth = -previous.z;
    bool previousInside = inside(previousDepth);
    for (const glm::vec3& current : input) {
        const float currentDepth = -current.z;
        const bool currentInside = inside(currentDepth);
        if (currentInside != previousInside) {
            const float t = (planeDepth - previousDepth) /
                            (currentDepth - previousDepth);
            output.push_back(previous + (current - previous) * t);
        }
        if (currentInside) output.push_back(current);
        previous = current;
        previousDepth = currentDepth;
        previousInside = currentInside;
    }
    return output;
}

std::vector<glm::vec3> ClipViewPlane(const std::vector<glm::vec3>& input,
                                     const glm::dvec3& normal) {
    std::vector<glm::vec3> output;
    if (input.empty()) return output;
    output.reserve(input.size() + 1);

    const auto distance = [&normal](const glm::vec3& point) {
        return glm::dot(normal, glm::dvec3(point));
    };
    glm::vec3 previous = input.back();
    double previousDistance = distance(previous);
    bool previousInside = previousDistance >= 0.0;
    for (const glm::vec3& current : input) {
        const double currentDistance = distance(current);
        const bool currentInside = currentDistance >= 0.0;
        if (currentInside != previousInside) {
            const double t = previousDistance /
                             (previousDistance - currentDistance);
            const glm::dvec3 intersection = glm::dvec3(previous) +
                (glm::dvec3(current) - glm::dvec3(previous)) * t;
            output.emplace_back(intersection);
        }
        if (currentInside) output.push_back(current);
        previous = current;
        previousDistance = currentDistance;
        previousInside = currentInside;
    }
    return output;
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
                                      glm::vec3(1.0f)), {}, false});
}

void Renderer3D::DrawPbrTriangle(const glm::vec3& a, const glm::vec3& b,
                                 const glm::vec3& c,
                                 const PBRMaterial& material) {
    if (!IsFinite(a) || !IsFinite(b) || !IsFinite(c) ||
        !IsFinite(material.baseColor) || !std::isfinite(material.metallic) ||
        !std::isfinite(material.roughness)) return;
    m_Triangles.push_back({a, b, c, {}, material, true});
}

void Renderer3D::SetPbrLighting(const glm::vec3& lightDirection,
                                const glm::vec3& lightRadiance,
                                const glm::vec3& ambientRadiance) {
    if (!IsFinite(lightDirection) || !IsFinite(lightRadiance) ||
        !IsFinite(ambientRadiance) ||
        glm::dot(lightDirection, lightDirection) <= 1.0e-12f)
        return;
    m_LightDirection = glm::normalize(lightDirection);
    m_LightRadiance = glm::max(lightRadiance, glm::vec3(0.0f));
    m_AmbientRadiance = glm::max(ambientRadiance, glm::vec3(0.0f));
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
    const double halfHeightSlope = std::tan(m_VerticalFovRadians * 0.5);
    const double halfWidthSlope = halfHeightSlope *
                                  static_cast<double>(viewport.x) /
                                  static_cast<double>(viewport.y);
    projected.reserve(m_Triangles.size());

    for (const Triangle& triangle : m_Triangles) {
        const glm::vec4 viewA = view * glm::vec4(triangle.a, 1.0f);
        const glm::vec4 viewB = view * glm::vec4(triangle.b, 1.0f);
        const glm::vec4 viewC = view * glm::vec4(triangle.c, 1.0f);
        if (!std::isfinite(viewA.x) || !std::isfinite(viewA.y) ||
            !std::isfinite(viewA.z) || !std::isfinite(viewB.x) ||
            !std::isfinite(viewB.y) || !std::isfinite(viewB.z) ||
            !std::isfinite(viewC.x) || !std::isfinite(viewC.y) ||
            !std::isfinite(viewC.z)) {
            continue;
        }

        std::vector<glm::vec3> polygon{glm::vec3(viewA), glm::vec3(viewB),
                                        glm::vec3(viewC)};
        polygon = ClipDepthPlane(polygon, m_NearPlane, true);
        polygon = ClipDepthPlane(polygon, m_FarPlane, false);
        polygon = ClipViewPlane(polygon, {1.0, 0.0, -halfWidthSlope});
        polygon = ClipViewPlane(polygon, {-1.0, 0.0, -halfWidthSlope});
        polygon = ClipViewPlane(polygon, {0.0, -1.0, -halfHeightSlope});
        polygon = ClipViewPlane(polygon, {0.0, 1.0, -halfHeightSlope});
        if (polygon.size() < 3) continue;

        for (std::size_t i = 1; i + 1 < polygon.size(); ++i) {
            const glm::vec3& a = polygon[0];
            const glm::vec3& b = polygon[i];
            const glm::vec3& c = polygon[i + 1];
            ProjectedTriangle result;
            if (!Project(projection, a, viewport, result.a) ||
                !Project(projection, b, viewport, result.b) ||
                !Project(projection, c, viewport, result.c)) {
                continue;
            }
            const float area = (result.b.x - result.a.x) *
                                   (result.c.y - result.a.y) -
                               (result.b.y - result.a.y) *
                                   (result.c.x - result.a.x);
            if (!std::isfinite(area) || std::abs(area) <= 1.0e-4f) continue;
            result.color = triangle.color;
            if (triangle.pbr) {
                const glm::vec3 edgeA = triangle.b - triangle.a;
                const glm::vec3 edgeB = triangle.c - triangle.a;
                const glm::vec3 normal = glm::cross(edgeA, edgeB);
                const glm::vec3 center =
                    (triangle.a + triangle.b + triangle.c) / 3.0f;
                result.color = ShadeMetallicRoughness(
                    triangle.material.baseColor, triangle.material.metallic,
                    triangle.material.roughness, normal, m_Position - center,
                    m_LightDirection, m_LightRadiance, m_AmbientRadiance);
            }
            result.depth = ((-a.z) + (-b.z) + (-c.z)) / 3.0f;
            projected.push_back(result);
        }
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
