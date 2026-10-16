#include "Renderer3D.h"

#include <cmath>
#include <cstdio>

using StonePotatoes::Rendering::Detail::Renderer3D;

int main() {
    Renderer3D renderer;
    if (!renderer.SetCamera({0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f},
                            {0.0f, 1.0f, 0.0f}, 1.57079632679f, 1.0f,
                            10.0f)) {
        std::fprintf(stderr, "valid perspective camera was rejected\n");
        return 1;
    }
    if (renderer.SetCamera({0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f},
                           {0.0f, 1.0f, 0.0f}, 1.0f, 0.1f, 10.0f)) {
        std::fprintf(stderr, "degenerate camera was accepted\n");
        return 2;
    }

    renderer.DrawTriangle({-2.0f, -1.0f, -4.0f}, {2.0f, -1.0f, -4.0f},
                          {0.0f, 2.0f, -4.0f}, {0.0f, 0.0f, 1.0f});
    renderer.DrawTriangle({-1.0f, -1.0f, -2.0f}, {1.0f, -1.0f, -2.0f},
                          {0.0f, 1.0f, -2.0f}, {2.0f, 0.0f, 0.0f});
    // Near/far-crossing faces are clipped at the camera planes instead of
    // being discarded just because one vertex lies outside the depth range.
    renderer.DrawTriangle({-2.0f, -1.0f, -0.5f}, {2.0f, -1.0f, -2.0f},
                          {0.0f, 2.0f, -2.0f}, {0.0f, 1.0f, 0.0f});
    renderer.DrawTriangle({-2.0f, -1.0f, -8.0f}, {2.0f, -1.0f, -11.0f},
                          {0.0f, 2.0f, -8.0f}, {1.0f, 0.5f, 0.0f});
    renderer.DrawTriangle({-1.0f, -1.0f, 0.5f}, {1.0f, -1.0f, 0.5f},
                          {0.0f, 1.0f, 0.5f}, {1.0f, 0.0f, 1.0f});

    const auto projected = renderer.ProjectTriangles({800.0f, 400.0f});
    const auto colorCount = [&projected](const glm::vec3& color) {
        std::size_t count = 0;
        for (const auto& triangle : projected) {
            if (triangle.color == color) ++count;
        }
        return count;
    };
    if (projected.size() != 6 || projected[0].depth <= projected[1].depth ||
        colorCount({0.0f, 1.0f, 0.0f}) != 2 ||
        colorCount({1.0f, 0.5f, 0.0f}) != 2) {
        std::fprintf(stderr, "frustum clipping or back-to-front sorting failed\n");
        return 3;
    }
    for (const auto& triangle : projected) {
        if (triangle.depth < 1.0f || triangle.depth > 10.0f) {
            std::fprintf(stderr, "clipped geometry escaped the depth range\n");
            return 4;
        }
    }
    bool redTriangleProjected = false;
    for (const auto& triangle : projected) {
        if (triangle.color.r == 1.0f && triangle.color.g == 0.0f &&
            triangle.color.b == 0.0f) {
            redTriangleProjected =
                std::abs(triangle.a.x + triangle.b.x - 800.0f) <= 0.01f;
            break;
        }
    }
    if (!redTriangleProjected) {
        std::fprintf(stderr, "perspective projection or color clamp failed\n");
        return 5;
    }

    renderer.Clear();
    renderer.DrawTriangle({-12.0f, -1.0f, -4.0f}, {-2.0f, -1.0f, -4.0f},
                          {-2.0f, 2.0f, -4.0f}, {0.0f, 1.0f, 1.0f});
    renderer.DrawTriangle({12.0f, -1.0f, -4.0f}, {2.0f, -1.0f, -4.0f},
                          {2.0f, 2.0f, -4.0f}, {1.0f, 0.0f, 1.0f});
    renderer.DrawTriangle({-1.0f, 6.0f, -4.0f}, {1.0f, 6.0f, -4.0f},
                          {0.0f, 1.0f, -4.0f}, {1.0f, 1.0f, 0.0f});
    renderer.DrawTriangle({-1.0f, -6.0f, -4.0f}, {1.0f, -6.0f, -4.0f},
                          {0.0f, -1.0f, -4.0f}, {0.0f, 1.0f, 0.0f});
    renderer.DrawTriangle({-24.0f, -1.0f, -4.0f}, {-20.0f, -1.0f, -4.0f},
                          {-22.0f, 2.0f, -4.0f}, {1.0f, 0.5f, 0.0f});
    renderer.DrawTriangle({20.0f, -1.0f, -4.0f}, {24.0f, -1.0f, -4.0f},
                          {22.0f, 2.0f, -4.0f}, {0.5f, 0.0f, 1.0f});
    renderer.DrawTriangle({-1.0f, 20.0f, -4.0f}, {1.0f, 20.0f, -4.0f},
                          {0.0f, 24.0f, -4.0f}, {1.0f, 0.5f, 1.0f});
    renderer.DrawTriangle({-1.0f, -20.0f, -4.0f}, {1.0f, -20.0f, -4.0f},
                          {0.0f, -24.0f, -4.0f}, {0.0f, 0.5f, 1.0f});
    const auto sideClipped = renderer.ProjectTriangles({800.0f, 400.0f});
    if (sideClipped.size() != 6) {
        std::fprintf(stderr, "side-frustum clipping expected 6 triangles, got %zu\n",
                     sideClipped.size());
        return 6;
    }
    for (const auto& triangle : sideClipped) {
        for (const glm::vec2 point : {triangle.a, triangle.b, triangle.c}) {
            if (!std::isfinite(point.x) || !std::isfinite(point.y) ||
                point.x < -0.01f || point.x > 800.01f ||
                point.y < -0.01f || point.y > 400.01f) {
                std::fprintf(stderr, "side clipping projected outside viewport\n");
                return 7;
            }
        }
    }

    renderer.Clear();
    if (!renderer.ProjectTriangles({800.0f, 400.0f}).empty()) {
        std::fprintf(stderr, "clear retained queued triangles\n");
        return 8;
    }
    return 0;
}
