// SPDX-License-Identifier: MIT
// Copyright (c) 2026 ArchaonHW
// Derived from PotatoEngine MathUtils/Vector2.h (MIT). See THIRD_PARTY_NOTICES.md.
// 衍生自 PotatoEngine MathUtils/Vector2.h（MIT），詳見 THIRD_PARTY_NOTICES.md。
#pragma once

#include <cmath>

namespace StonePotatoes {
namespace Math {

// Minimal 2D vector (for pathfinding/procedural use). Header-only.
// 最小化 2D 向量（用於尋路/程序化），header-only。
struct Vec2 {
    float x = 0.0f, y = 0.0f;

    Vec2() = default;
    Vec2(float x_, float y_) : x(x_), y(y_) {}

    Vec2 operator+(const Vec2& o) const { return Vec2(x + o.x, y + o.y); }
    Vec2 operator-(const Vec2& o) const { return Vec2(x - o.x, y - o.y); }
    Vec2 operator*(float s) const { return Vec2(x * s, y * s); }
    Vec2 operator/(float s) const { return Vec2(x / s, y / s); }

    Vec2& operator+=(const Vec2& o) { x += o.x; y += o.y; return *this; }
    Vec2& operator-=(const Vec2& o) { x -= o.x; y -= o.y; return *this; }
    Vec2& operator*=(float s) { x *= s; y *= s; return *this; }

    float Dot(const Vec2& o) const { return x * o.x + y * o.y; }
    float Length() const { return std::sqrt(x * x + y * y); }
    float LengthSquared() const { return x * x + y * y; }
    Vec2 Normalize() const {
        float len = Length();
        return len > 0.0f ? Vec2(x / len, y / len) : Vec2();
    }
};

} // namespace Math
} // namespace StonePotatoes
