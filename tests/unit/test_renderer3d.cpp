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
    renderer.DrawTriangle({-1.0f, -1.0f, 0.5f}, {1.0f, -1.0f, 0.5f},
                          {0.0f, 1.0f, 0.5f}, {0.0f, 1.0f, 0.0f});

    const auto projected = renderer.ProjectTriangles({800.0f, 400.0f});
    if (projected.size() != 2 || projected[0].depth <= projected[1].depth) {
        std::fprintf(stderr, "visible triangles were not depth sorted/cull filtered\n");
        return 3;
    }
    if (projected[1].color.r != 1.0f || projected[1].color.g != 0.0f ||
        std::abs(projected[1].a.x + projected[1].b.x - 800.0f) > 0.01f) {
        std::fprintf(stderr, "perspective projection or color clamp failed\n");
        return 4;
    }

    renderer.Clear();
    if (!renderer.ProjectTriangles({800.0f, 400.0f}).empty()) {
        std::fprintf(stderr, "clear retained queued triangles\n");
        return 5;
    }
    return 0;
}
