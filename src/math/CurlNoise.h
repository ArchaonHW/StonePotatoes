// SPDX-License-Identifier: MIT
// Copyright (c) 2026 ArchaonHW
// Derived from PotatoEngine MathUtils/CurlNoise.h (MIT). See THIRD_PARTY_NOTICES.md.
// 衍生自 PotatoEngine MathUtils/CurlNoise.h（MIT），詳見 THIRD_PARTY_NOTICES.md。
#pragma once

#include "Vec3.h"
#include "QuasiRandom.h"

#include <cmath>
#include <cstdint>
#include <vector>

namespace StonePotatoes {
namespace Math {
namespace Quasi {

// Divergence-free turbulence field (curl noise / synthetic turbulence).
// Builds from analytic sinusoidal superposition: v = ∇×ψ where ψ is a
// vector potential. Exact zero divergence (not a finite-difference approx).
// Wave directions use R3 quasi-random sequence to avoid stripes.
// Deterministic for a given seed.
// 無散度湍流場（curl noise / synthetic turbulence）。
// 由解析正弦疊加構建：v = ∇×ψ，散度嚴格為零（非有限差分近似）。
// 波向量方向取自 R3 準隨機序列避免條紋。固定 seed 決定性。

class TurbulenceField {
public:
    explicit TurbulenceField(int octaves = 6, uint64_t seed = 0,
                             float baseFrequency = 0.15f,
                             float baseAmplitude = 1.0f) {
        Build(octaves, seed, baseFrequency, baseAmplitude);
    }

    // Sample velocity at world position p, time t | 取樣位置 p 時間 t 的速度
    Vec3 Sample(const Vec3& p, float t = 0.0f) const {
        Vec3 v(0.0f, 0.0f, 0.0f);
        for (const Octave& o : m_Waves) {
            // v = ∇×ψ; ψ_c = A_c sin(s), s = k·p + ωt + φ_c
            const double s = static_cast<double>(o.k.x) * p.x +
                             static_cast<double>(o.k.y) * p.y +
                             static_cast<double>(o.k.z) * p.z +
                             o.omega * t;
            const double cx = std::cos(s + o.phase.x);
            const double cy = std::cos(s + o.phase.y);
            const double cz = std::cos(s + o.phase.z);
            // curl components:
            v.x += static_cast<float>(o.k.y * o.amp.z * cz - o.k.z * o.amp.y * cy);
            v.y += static_cast<float>(o.k.z * o.amp.x * cx - o.k.x * o.amp.z * cz);
            v.z += static_cast<float>(o.k.x * o.amp.y * cy - o.k.y * o.amp.x * cx);
        }
        return v;
    }

    // Divergence verification (finite-difference). Should be ~0 (~1e-5).
    // 散度驗證（有限差分估算），應≈0（精度 ~1e-5）。
    double EstimateDivergence(const Vec3& p, float t, float eps = 1e-3f) const {
        const Vec3 dx = Sample(p + Vec3(eps, 0, 0), t) - Sample(p - Vec3(eps, 0, 0), t);
        const Vec3 dy = Sample(p + Vec3(0, eps, 0), t) - Sample(p - Vec3(0, eps, 0), t);
        const Vec3 dz = Sample(p + Vec3(0, 0, eps), t) - Sample(p - Vec3(0, 0, eps), t);
        return static_cast<double>(dx.x + dy.y + dz.z) / (2.0 * eps);
    }

private:
    struct Octave {
        Vec3 k;      // wave vector (direction from R3, length = frequency)
        Vec3 amp;    // amplitude per potential component
        Vec3 phase;  // phase per potential component
        float omega; // angular frequency (time)
    };

    void Build(int octaves, uint64_t seed, float baseFreq, float baseAmp) {
        m_Waves.clear();
        std::mt19937_64 rng(seed);
        std::uniform_real_distribution<float> u01(0.0f, 1.0f);

        // R3 inverse plastic constants (ψ₃ ≈ 1.2207)
        constexpr double g1 = 0.8191725133961644;  // 1/ψ₃
        constexpr double g2 = 0.6710436067037893;  // 1/ψ₃²

        float freq = baseFreq, amp = baseAmp;
        for (int i = 0; i < octaves; ++i) {
            Octave o;
            // Direction: R3 point mapped to sphere (z = 2u-1, φ = 2πv)
            const double u = detail::Frac((i + seed) * g1);
            const double vv = detail::Frac((i + seed) * g2);
            const double z = 2.0 * u - 1.0;
            const double r = std::sqrt(std::max(0.0, 1.0 - z * z));
            const double az = 6.283185307179586 * vv;
            o.k = Vec3(static_cast<float>(r * std::cos(az) * freq),
                       static_cast<float>(r * std::sin(az) * freq),
                       static_cast<float>(z * freq));
            // Independent amplitudes/phases for turbulence appearance
            o.amp = Vec3(amp * u01(rng), amp * u01(rng), amp * u01(rng));
            o.phase = Vec3(6.2831853f * u01(rng), 6.2831853f * u01(rng), 6.2831853f * u01(rng));
            o.omega = u01(rng) * 0.5f;
            m_Waves.push_back(o);
            freq *= 2.0f;
            amp *= 0.5f;
        }
    }

    std::vector<Octave> m_Waves;
};

} // namespace Quasi
} // namespace Math
} // namespace StonePotatoes
