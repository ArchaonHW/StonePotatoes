#pragma once

#include <algorithm>
#include <cmath>
#include <complex>
#include <cstdint>
#include <vector>

// RiemannZeta — Riemann ζ / L-function numerical core (headless, std only).
// Ported from PotatoEngine MathUtils/RiemannZeta.h (the local RH finite
// verification work) and extended with a Riemann–Siegel C0 remainder,
// Gram points and sieve helpers so StonePotatoes can scan high t cheaply.
// IMPORTANT: everything here is FINITE NUMERICAL VERIFICATION — it can
// falsify RH up to a height, it can never prove it.
// RiemannZeta — 黎曼 ζ/L 函數數值核心（headless，僅標準庫）。
// 移植自 PotatoEngine MathUtils/RiemannZeta.h（本機 RH 有限驗證成果），
// 並擴充 Riemann–Siegel C0 餘項、Gram 點與篩法工具，
// 讓 StonePotatoes 能以 O(√t) 代價掃描高 t。
// 重要：此處全部是「有限數值驗證」——可在某高度內證偽 RH，永遠無法證明它。
namespace StonePotatoes {
namespace Math {
namespace Zeta {

using C = std::complex<double>;
inline constexpr double kPi = 3.14159265358979323846;
inline constexpr double kTwoPi = 2.0 * kPi;
inline constexpr double kEulerGamma = 0.57721566490153286061;

// n^{-s} = n^{-σ}·e^{-it ln n} (real base complex power — faster and
// more stable than std::pow(complex))
// n^{-s} = n^{-σ}·e^{-it ln n}（實底複指數，比 std::pow(complex) 快且穩）
inline C PowNeg(double n, const C& s) {
    const double ln = std::log(n);
    const double mag = std::exp(-s.real() * ln);
    const double ph = -s.imag() * ln;
    return C(mag * std::cos(ph), mag * std::sin(ph));
}

// Euler–Maclaurin correction coefficients B_{2k}/(2k)!
// Euler–Maclaurin 修正項係數 B_{2k}/(2k)!（ZetaEMN / HurwitzEMN 共用）
inline constexpr int kEmM = 8;
inline constexpr double kEmCoef[kEmM] = {
    (1.0 / 6) / 2, (-1.0 / 30) / 24, (1.0 / 42) / 720,
    (-1.0 / 30) / 40320, (5.0 / 66) / 3628800,
    (-691.0 / 2730) / 479001600.0, (7.0 / 6) / 87178291200.0,
    (-3617.0 / 510) / 20922789888000.0,
};

// Euler–Maclaurin ζ(s) with N terms. High t needs N ≳ 4t, otherwise the
// asymptotic tail diverges.
// Euler–Maclaurin ζ(s)，N 參數化；高 t 需 N≳4t，否則漸近發散。
inline C ZetaEMN(const C& s, int N) {
    C sum = 0.0;
    for (int n = 1; n < N; ++n) sum += PowNeg(n, s);
    sum += PowNeg(N, s - C(1, 0)) / (s - C(1, 0)); // N^{1-s}/(s-1)
    sum += 0.5 * PowNeg(N, s);                     // boundary | 邊界項
    C rise = s; // rising factorial (s)_{2k-1} | 升冪階乘
    for (int k = 1; k <= kEmM; ++k) {
        sum += kEmCoef[k - 1] * rise * PowNeg(N, s + C(2 * k - 1, 0));
        rise *= (s + C(2 * k - 1, 0)) * (s + C(2 * k, 0));
    }
    return sum;
}

// Hurwitz ζ(s,a) = Σ_{n≥0} (n+a)^{-s}, same EM machinery.
// Hurwitz ζ(s,a)：同一套 EM，截斷點在 x=N+a。
inline C HurwitzEMN(const C& s, double a, int N) {
    C sum = 0.0;
    for (int n = 0; n < N; ++n) sum += PowNeg(n + a, s);
    const double x = N + a;
    sum += C(x) * PowNeg(x, s) / (s - C(1, 0));
    sum += 0.5 * PowNeg(x, s);
    C rise = s;
    for (int k = 1; k <= kEmM; ++k) {
        sum += kEmCoef[k - 1] * rise * PowNeg(x, s + C(2 * k - 1, 0));
        rise *= (s + C(2 * k - 1, 0)) * (s + C(2 * k, 0));
    }
    return sum;
}

// EM term count that keeps the expansion convergent at height |t|.
// 讓 EM 在高度 |t| 保持收斂的項數。
inline int EmTermsFor(double t) {
    return std::max(200, static_cast<int>(4.2 * std::fabs(t)));
}

inline C ZetaEM(const C& s) { return ZetaEMN(s, EmTermsFor(s.imag())); }

// Complex Γ: Lanczos (g=7, n=9) + reflection for Re z < 0.5.
// 複數 Γ：Lanczos（g=7, n=9）+ 反射公式。
inline C Gamma(const C& z) {
    static constexpr double c[9] = {
        0.99999999999980993, 676.5203681218851,
        -1259.1392167224028, 771.32342877765313,
        -176.61502916214059, 12.507343278686905,
        -0.13857109526572012, 9.9843695780195716e-6,
        1.5056327351493116e-7,
    };
    if (z.real() < 0.5)
        return C(kPi) / (std::sin(kPi * z) * Gamma(C(1.0) - z));
    const C zm1 = z - C(1.0);
    C a = c[0];
    for (int k = 1; k < 9; ++k) a += c[k] / (zm1 + C(k, 0));
    const C t = zm1 + C(7.5, 0);
    return std::sqrt(kTwoPi) * std::pow(t, zm1 + C(0.5, 0)) *
           std::exp(-t) * a;
}

// Functional equation factor: ζ(s) = χ(s)·ζ(1-s)
// 函數方程式因子：χ(s) = 2^s π^{s-1} sin(πs/2) Γ(1-s)
inline C Chi(const C& s) {
    return std::pow(C(2.0), s) * std::pow(C(kPi), s - C(1.0)) *
           std::sin(kPi * 0.5 * s) * Gamma(C(1.0) - s);
}

// Riemann–Siegel θ(t) asymptotic expansion (t ≳ 10: ~1e-6 rad; t ≥ 1 valid).
// Riemann–Siegel θ(t) 漸近展開（t≳10 精度 ~1e-6 rad；需 t≥1）。
inline double RSTheta(double t) {
    return t * 0.5 * std::log(t / kTwoPi) - t * 0.5 - kPi / 8.0 +
           1.0 / (48.0 * t) + 7.0 / (5760.0 * t * t * t);
}

// θ'(t) ≈ ½ ln(t/2π) (Newton step for Gram points)
// θ'(t) ≈ ½ ln(t/2π)（Gram 點牛頓迭代用）
inline double RSThetaPrime(double t) {
    return 0.5 * std::log(t / kTwoPi) - 1.0 / (48.0 * t * t);
}

// Riemann–Siegel main sum only: 2 Σ_{n≤√(t/2π)} cos(θ(t)-t ln n)/√n
// Riemann–Siegel 主和（無餘項）
inline double RSZ(double t) {
    const int n = static_cast<int>(std::sqrt(t / kTwoPi));
    const double th = RSTheta(t);
    double z = 0.0;
    for (int k = 1; k <= n; ++k)
        z += std::cos(th - t * std::log(static_cast<double>(k))) /
             std::sqrt(static_cast<double>(k));
    return 2.0 * z;
}

// Riemann–Siegel C0(p) = cos(2π(p²−p−1/16)) / cos(2πp).
// Removable singularities at p = 1/4, 3/4 → symmetric average.
// Riemann–Siegel C0(p)；p=1/4、3/4 為可去奇點 → 對稱平均。
inline double RSC0(double p) {
    auto raw = [](double q) {
        return std::cos(kTwoPi * (q * q - q - 1.0 / 16.0)) /
               std::cos(kTwoPi * q);
    };
    if (std::fabs(std::cos(kTwoPi * p)) < 1e-5) {
        constexpr double d = 1e-4;
        return 0.5 * (raw(p - d) + raw(p + d));
    }
    return raw(p);
}

// Hardy Z(t) via Riemann–Siegel main sum + C0 remainder.
// Cost O(√t); error ~O(t^{-3/4}) — sign structure is reliable for t ≳ 100.
// Hardy Z(t)：Riemann–Siegel 主和 + C0 餘項。
// 代價 O(√t)；誤差約 O(t^{-3/4})——t≳100 變號結構可靠。
inline double RSZCorrected(double t) {
    const double a = std::sqrt(t / kTwoPi);
    const int n = static_cast<int>(a);
    const double p = a - n;
    const double th = RSTheta(t);
    double z = 0.0;
    for (int k = 1; k <= n; ++k)
        z += std::cos(th - t * std::log(static_cast<double>(k))) /
             std::sqrt(static_cast<double>(k));
    const double sign = ((n - 1) % 2 == 0) ? 1.0 : -1.0;
    return 2.0 * z + sign * std::pow(a, -0.5) * RSC0(p);
}

// High-t rotated ζ: Re(e^{iθ(t)} ζ(½+it)), N ≈ 4t.
// 高 t 旋轉 ζ：Re(e^{iθ(t)}ζ(½+it))，N≈4t 避免 EM 漸近發散。
inline double RotZetaHi(double t, int bigN) {
    const double th = RSTheta(t);
    const C e(std::cos(th), std::sin(th));
    return (e * ZetaEMN(C(0.5, t), bigN)).real();
}

// Rotated ζ, EM precision. Imag part ≈ 0 is itself an on-line check.
// 旋轉 ζ（EM 精度版）；虛部應 ~0——本身即「此 t 在線上」檢查。
inline double RotZeta(double t) { return RotZetaHi(t, EmTermsFor(t)); }

// Hybrid Hardy Z(t): EM (accurate) below hybridT, Riemann–Siegel above.
// 混合 Hardy Z(t)：hybridT 以下用 EM（精確），以上用 Riemann–Siegel。
inline constexpr double kHybridT = 400.0;
inline double HardyZ(double t) {
    return t < kHybridT ? RotZeta(t) : RSZCorrected(t);
}

// Smooth zero-counting function N₀(T) = θ(T)/π + 1 (Riemann–von Mangoldt
// main term; true N(T) = N₀(T) + S(T)).
// 平滑零點計數 N₀(T)=θ(T)/π+1（真 N(T)=N₀(T)+S(T)）。
inline double SmoothZeroCount(double T) { return RSTheta(T) / kPi + 1.0; }

// Gram point g_n: θ(g_n) = nπ (n ≥ -1). θ is convex increasing for t > 2π,
// so Newton started right of the root converges monotonically.
// Gram 點 g_n：θ(g_n)=nπ（n≥-1）。θ 在 t>2π 凸遞增，
// 自根右側起的牛頓法單調收斂。
inline double GramPoint(int64_t n) {
    const double target = static_cast<double>(n) * kPi;
    double t = kTwoPi * static_cast<double>(std::max<int64_t>(n, 0)) + 100.0;
    for (int i = 0; i < 100; ++i) {
        const double dt = (RSTheta(t) - target) / RSThetaPrime(t);
        t -= dt;
        if (std::fabs(dt) < 1e-12 * t) break;
    }
    return t;
}

// Dirichlet β(s) = L(s,χ₄) via Euler transform.
// Dirichlet β(s) = L(s,χ₄)，Euler 變換加速。
inline C BetaDirichlet(const C& s) {
    const int K = 150;
    std::vector<C> a(static_cast<size_t>(K) + 1);
    for (int n = 0; n <= K; ++n) a[static_cast<size_t>(n)] = PowNeg(2 * n + 1, s);
    C sum = 0.0;
    double denom = 2.0;
    for (int k = 0; k <= K; ++k) {
        sum += a[0] / denom;
        denom *= 2.0;
        for (int i = 0; i < K - k; ++i)
            a[static_cast<size_t>(i)] -= a[static_cast<size_t>(i + 1)];
    }
    return sum;
}

// Dirichlet η(s) = Σ (−1)^{n−1} n^{−s}; η = (1−2^{1−s})ζ cross-check path.
// Dirichlet η(s)；η=(1−2^{1−s})ζ 提供雙路徑交叉驗證。
inline C EtaEuler(const C& s) {
    const int K = 120;
    std::vector<C> a(static_cast<size_t>(K) + 1);
    for (int n = 1; n <= K + 1; ++n) a[static_cast<size_t>(n - 1)] = PowNeg(n, s);
    C sum = 0.0;
    double denom = 2.0;
    for (int k = 0; k <= K; ++k) {
        sum += a[0] / denom;
        denom *= 2.0;
        for (int i = 0; i < K - k; ++i)
            a[static_cast<size_t>(i)] -= a[static_cast<size_t>(i + 1)];
    }
    return sum;
}

// ξ(s) = ½s(s−1)π^{−s/2}Γ(s/2)ζ(s): entire, zeros = non-trivial zeros.
// ξ(s)：整函數，零點恰為非平凡零點；線上實值。
inline C Xi(const C& s) {
    return C(0.5) * s * (s - C(1.0)) * std::pow(C(kPi), -s * C(0.5)) *
           Gamma(s * C(0.5)) * ZetaEM(s);
}

// de Bruijn–Newman kernel Φ(u) (even); H_t zeros real for t > Λ; RH ⇔ Λ = 0.
// de Bruijn–Newman 核 Φ(u)（偶函數）；RH ⇔ Λ=0。
inline double PhiBN(double u) {
    if (u < 0.0) u = -u;
    const double e4 = std::exp(4.0 * u);
    const double e5 = std::exp(5.0 * u);
    const double e9 = e5 * e4;
    double s = 0.0;
    for (int n = 1; n <= 30; ++n) {
        const double n2 = static_cast<double>(n) * n;
        const double damp = std::exp(-kPi * n2 * e4);
        if (damp < 1e-30) break;
        s += (2.0 * kPi * kPi * n2 * n2 * e9 - 3.0 * kPi * n2 * e5) * damp;
    }
    return s;
}

// H_t(x) = ∫ e^{tu²}Φ(u)cos(xu)du (trapezoid, spectrally accurate).
// Valid for |x| ≲ 85 before hitting the float noise floor.
// H_t(x) 梯形積分；|x|≲85 以內有效（再高進入浮點噪底）。
inline double HtFlow(double x, double t) {
    const double h = 0.015, uMax = 2.5;
    const int N = static_cast<int>(uMax / h);
    double s = 0.0;
    for (int i = -N; i <= N; ++i) {
        const double u = i * h;
        s += PhiBN(u) * std::exp(t * u * u) * std::cos(x * u);
    }
    return s * h;
}

// Bisection root of a sign change in [a,b].
// 變號區間二分求根。
template <class F>
inline double Bisect(F f, double a, double b, double tol) {
    double fa = f(a);
    while (b - a > tol) {
        const double m = 0.5 * (a + b);
        const double fm = f(m);
        if (fa * fm <= 0.0) {
            b = m;
        } else {
            a = m;
            fa = fm;
        }
    }
    return 0.5 * (a + b);
}

// Sign-change scan over [lo,hi] with refinement → root list.
// 變號掃描 + 二分求精 → 根列表。
template <class F>
inline std::vector<double> ScanRoots(F f, double lo, double hi, double step) {
    std::vector<double> roots;
    double xp = lo, fp = f(lo);
    const int64_t steps = static_cast<int64_t>((hi - lo) / step);
    for (int64_t i = 1; i <= steps; ++i) {
        const double x = lo + static_cast<double>(i) * step;
        const double fx = f(x);
        if (fp * fx < 0.0) roots.push_back(Bisect(f, xp, x, step * 1e-6));
        xp = x;
        fp = fx;
    }
    return roots;
}

// Argument principle: winding of f around rectangle → zero count inside.
// 輻角原理：沿矩形逆時針一圈的環繞數 → 矩形內零點數。
template <class F>
inline int ArgPrincipleCount(F f, double sLo, double sHi, double tLo,
                             double tHi, double step) {
    double winding = 0.0, prevArg = 0.0;
    bool first = true;
    auto sample = [&](double s, double t) {
        const C v = f(C(s, t));
        const double arg = std::atan2(v.imag(), v.real());
        if (!first) {
            double d = arg - prevArg;
            while (d > kPi) d -= kTwoPi;
            while (d < -kPi) d += kTwoPi;
            winding += d;
        }
        prevArg = arg;
        first = false;
    };
    const double sLen = sHi - sLo, tLen = tHi - tLo;
    const int sSteps = std::max(2, static_cast<int>(sLen / step));
    const int tSteps = std::max(2, static_cast<int>(tLen / step));
    for (int i = 0; i <= sSteps; ++i) sample(sLo + sLen * i / sSteps, tLo);
    for (int i = 1; i <= tSteps; ++i) sample(sHi, tLo + tLen * i / tSteps);
    for (int i = 1; i <= sSteps; ++i) sample(sHi - sLen * i / sSteps, tHi);
    for (int i = 1; i < tSteps; ++i) sample(sLo, tHi - tLen * i / tSteps);
    sample(sLo, tLo);
    return static_cast<int>(std::lround(winding / kTwoPi));
}

// ---------------------------------------------------------------------------
// Arithmetic side (sieve-based) | 算術側（篩法）
// ---------------------------------------------------------------------------

// Chebyshev ψ(x) = Σ_{p^k ≤ x} ln p.
// Chebyshev ψ(x)——顯式公式比對目標。
double PsiChebyshev(double x);

// Truncated explicit formula with Fejér weights:
// ψ₀(x) ≈ x − ln 2π − ½ln(1−x⁻²) − Σ_{γ<T}(1−γ/T)·2Re(x^ρ/ρ)
// 顯式公式截斷（Fejér 權重）；gammas 為升冪正虛部零點。
double PsiExplicit(double x, const std::vector<double>& gammas, double T);

// Möbius μ(n) for n ∈ [0, n] (linear sieve). μ(0) = 0.
// Möbius μ 線性篩。
std::vector<int8_t> MobiusSieve(uint32_t n);

// Mertens: max_{2≤x≤n} |M(x)|/√x (RH ⇒ M(x) = O(x^{1/2+ε})).
// Mertens 比值最大值。
double MertensMaxRatio(uint32_t n, uint32_t* argMax = nullptr);

// Robin: max_{5041≤m≤n} σ(m)/(e^γ m ln ln m). RH ⇔ < 1 for all m > 5040.
// Robin 不等式比值最大值；RH ⇔ 對所有 m>5040 皆 <1。
double RobinMaxRatio(uint32_t n, uint32_t* argMax = nullptr);

// Li-coefficient truncation λ₁ ≈ Σ_ρ [1 − (1−1/ρ)^1] over given zeros
// (pairs ρ, ρ̄). Li criterion: RH ⇔ λ_n ≥ 0 ∀n.
// Li 係數 λ_n 截斷和（共軛成對）；Li 判準：RH ⇔ λ_n≥0。
double LiLambdaTruncated(int n, const std::vector<double>& gammas);

} // namespace Zeta
} // namespace Math
} // namespace StonePotatoes
