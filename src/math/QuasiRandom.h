// SPDX-License-Identifier: MIT
// Copyright (c) 2026 ArchaonHW
// Derived from PotatoEngine MathUtils/QuasiRandom.h (MIT). See THIRD_PARTY_NOTICES.md.
// 衍生自 PotatoEngine MathUtils/QuasiRandom.h（MIT），詳見 THIRD_PARTY_NOTICES.md。
#pragma once

#include "Vec2.h"

#include <cmath>
#include <cstdint>
#include <random>
#include <vector>

namespace StonePotatoes {
namespace Math {
namespace Quasi {

// Quasi-random (low-discrepancy) sequences & blue-noise sampling.
// Uses R-sequence (plastic constants) and Bridson 2007 Poisson-disk.
// Deterministic for a given seed.
// 準隨機（低差異）序列與藍噪取樣。使用 R-sequence（塑性常數）與
// Bridson 2007 Poisson-disk。固定 seed 決定性。

namespace detail {
inline double Frac(double v) { return v - std::floor(v); }
} // namespace detail

// van der Corput in base 'b' (radix-reversed n)
inline double VanDerCorput(uint64_t n, int base = 2) {
    double result = 0.0;
    double denom = 1.0;
    double b = static_cast<double>(base);
    while (n > 0) {
        denom *= b;
        result += static_cast<double>(n % static_cast<uint64_t>(base)) / denom;
        n /= static_cast<uint64_t>(base);
    }
    return result;
}

// Halton: dim-th dimension uses the i-th prime (2,3,5,7,11,...)
inline double Halton(uint64_t n, int dim) {
    static const int kPrimes[] = {2, 3, 5, 7, 11, 13, 17, 19, 23, 29,
                                  31, 37, 41, 43, 47, 53};
    const int base = kPrimes[dim % 16];
    return VanDerCorput(n + 1, base);
}

// R2 (2D plastic-constant sequence): returns the n-th point in [0,1)^2
inline Vec2 R2(uint64_t n, uint64_t skip = 0) {
    constexpr double kInvPsi2   = 0.7548776662466927;  // 1/ψ₂
    constexpr double kInvPsi2_2 = 0.5698402909980532;  // 1/ψ₂²
    const double i = static_cast<double>(n + skip);
    return Vec2(
        static_cast<float>(detail::Frac(i * kInvPsi2)),
        static_cast<float>(detail::Frac(i * kInvPsi2_2)));
}

// R2 mapped to a disk of radius radius (area-uniform)
inline Vec2 R2InDisk(uint64_t n, float radius, uint64_t skip = 0) {
    const Vec2 p = R2(n, skip);
    const float r = radius * std::sqrt(p.x);
    const float theta = 2.0f * 3.14159265358979323846f * p.y;
    return Vec2(r * std::cos(theta), r * std::sin(theta));
}

// Poisson-disk sampling (Bridson 2007) in a disk: generate count points,
// any two at least minDist apart. Uses spatial hash. Seeded -> reproducible.
// Returns <= count points (may saturate if minDist too large).
inline std::vector<Vec2> PoissonDisk(Vec2 center, float radius,
                                       int count, float minDist,
                                       uint64_t seed = 0) {
    std::vector<Vec2> points;
    if (count <= 0 || radius <= 0.0f || minDist <= 0.0f) return points;

    std::mt19937_64 rng(seed);
    std::uniform_real_distribution<float> u01(0.0f, 1.0f);

    const float cell = minDist / 1.4142135623730951f;
    const float inv = 1.0f / cell;
    const int gridSide = static_cast<int>(std::ceil(2.0f * radius * inv)) + 2;
    std::vector<int> grid(static_cast<size_t>(gridSide * gridSide), -1);
    const Vec2 origin(center.x - radius, center.y - radius);

    auto gridIndex = [&](const Vec2& p) -> int {
        int gx = static_cast<int>((p.x - origin.x) * inv);
        int gy = static_cast<int>((p.y - origin.y) * inv);
        if (gx < 0 || gy < 0 || gx >= gridSide || gy >= gridSide) return -1;
        return gy * gridSide + gx;
    };

    auto farFromAll = [&](const Vec2& p) -> bool {
        int gx = static_cast<int>((p.x - origin.x) * inv);
        int gy = static_cast<int>((p.y - origin.y) * inv);
        for (int dy = -2; dy <= 2; ++dy) {
            for (int dx = -2; dx <= 2; ++dx) {
                int nx = gx + dx, ny = gy + dy;
                if (nx < 0 || ny < 0 || nx >= gridSide || ny >= gridSide) continue;
                int idx = grid[static_cast<size_t>(ny * gridSide + nx)];
                if (idx < 0) continue;
                const Vec2 d = points[static_cast<size_t>(idx)] - p;
                if (d.LengthSquared() < minDist * minDist) return false;
            }
        }
        return true;
    };

    const int maxAttempts = count * 30;
    for (int attempt = 0; attempt < maxAttempts &&
         static_cast<int>(points.size()) < count; ++attempt) {
        const float r = radius * std::sqrt(u01(rng));
        const float theta = 2.0f * 3.14159265358979323846f * u01(rng);
        Vec2 p(center.x + r * std::cos(theta), center.y + r * std::sin(theta));
        if (!farFromAll(p)) continue;
        int gi = gridIndex(p);
        if (gi >= 0) grid[static_cast<size_t>(gi)] = static_cast<int>(points.size());
        points.push_back(p);
    }
    return points;
}

// Simple L2 discrepancy proxy: RMS deviation of grid counts from area proportion.
// Non-rigorous but enough to distinguish quasi-random from PRNG uniformity.
inline double Discrepancy2D(const std::vector<Vec2>& points, int gridDivisions = 16) {
    if (points.empty() || gridDivisions <= 0) return 0.0;
    const int n = gridDivisions;
    const double cellArea = 1.0 / (n * n);
    const double total = static_cast<double>(points.size());
    std::vector<int> counts(static_cast<size_t>(n * n), 0);
    for (const Vec2& p : points) {
        int gx = static_cast<int>(p.x * n);
        int gy = static_cast<int>(p.y * n);
        gx = (gx < 0) ? 0 : ((gx >= n) ? n - 1 : gx);
        gy = (gy < 0) ? 0 : ((gy >= n) ? n - 1 : gy);
        ++counts[static_cast<size_t>(gy * n + gx)];
    }
    double sumSq = 0.0;
    for (int c : counts) {
        const double dev = c / total - cellArea;
        sumSq += dev * dev;
    }
    return std::sqrt(sumSq / (n * n));
}

} // namespace Quasi
} // namespace Math
} // namespace StonePotatoes
