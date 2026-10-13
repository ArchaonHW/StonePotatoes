// SPDX-License-Identifier: MIT
// Copyright (c) 2026 ArchaonHW
// Derived from PotatoEngine MathUtils/Vector3.h (MIT). See THIRD_PARTY_NOTICES.md.
// 衍生自 PotatoEngine MathUtils/Vector3.h（MIT），詳見 THIRD_PARTY_NOTICES.md。
#pragma once

#include <cmath>

namespace StonePotatoes {
namespace Math {

// Minimal 3D vector (for turbulence/CurlNoise). Header-only.
// 最小化 3D 向量（用於湍流/CurlNoise），header-only。
struct Vec3 {
    float x = 0.0f, y = 0.0f, z = 0.0f;

    Vec3() = default;
    Vec3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}

    Vec3 operator+(const Vec3& o) const { return Vec3(x + o.x, y + o.y, z + o.z); }
    Vec3 operator-(const Vec3& o) const { return Vec3(x - o.x, y - o.y, z - o.z); }
    Vec3 operator*(float s) const { return Vec3(x * s, y * s, z * s); }
    Vec3 operator/(float s) const { return Vec3(x / s, y / s, z / s); }

    Vec3& operator+=(const Vec3& o) { x += o.x; y += o.y; z += o.z; return *this; }
    Vec3& operator-=(const Vec3& o) { x -= o.x; y -= o.y; z -= o.z; return *this; }

    float Dot(const Vec3& o) const { return x * o.x + y * o.y + z * o.z; }
    float Length() const { return std::sqrt(x * x + y * y + z * z); }
    float LengthSquared() const { return x * x + y * y + z * z; }
    Vec3 Normalize() const {
        float len = Length();
        return len > 0.0f ? Vec3(x / len, y / len, z / len) : Vec3();
    }
};

} // namespace Math
} // namespace StonePotatoes
