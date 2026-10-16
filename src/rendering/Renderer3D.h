#pragma once

#include <glm/glm.hpp>

#include <vector>

struct ImDrawList;

namespace StonePotatoes::Rendering::Detail {

struct ProjectedTriangle {
    glm::vec2 a{};
    glm::vec2 b{};
    glm::vec2 c{};
    glm::vec3 color{};
    float depth = 0.0f;
};

class Renderer3D final {
public:
    bool SetCamera(const glm::vec3& position, const glm::vec3& target,
                   const glm::vec3& up, float verticalFovRadians,
                   float nearPlane, float farPlane);
    void DrawTriangle(const glm::vec3& a, const glm::vec3& b,
                      const glm::vec3& c, const glm::vec3& color);

    std::vector<ProjectedTriangle> ProjectTriangles(
        const glm::vec2& viewport) const;
    void Flush(ImDrawList* drawList, const glm::vec2& viewport);
    void Clear();

private:
    struct Triangle {
        glm::vec3 a{};
        glm::vec3 b{};
        glm::vec3 c{};
        glm::vec3 color{};
    };

    glm::vec3 m_Position{0.0f, 0.0f, 5.0f};
    glm::vec3 m_Target{0.0f, 0.0f, 0.0f};
    glm::vec3 m_Up{0.0f, 1.0f, 0.0f};
    float m_VerticalFovRadians = 1.04719755f;
    float m_NearPlane = 0.1f;
    float m_FarPlane = 1000.0f;
    std::vector<Triangle> m_Triangles;
};

} // namespace StonePotatoes::Rendering::Detail
