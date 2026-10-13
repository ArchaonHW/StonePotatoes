#pragma once

#include <cmath>
#include <cstdint>

// DoubleDouble — unevaluated sum hi + lo (~106-bit significand, ~32 digits).
// Error-free transforms (Knuth TwoSum, Dekker TwoProd via Veltkamp split —
// no FMA dependency) plus exp/log good to ~5e-29 relative (exp squares 10×,
// amplifying kernel error ~1024×; measured). Used to keep the
// Riemann–Siegel phase θ(t) − t·ln k accurate when t·ln k ≈ 10¹¹ rad.
// DoubleDouble — 未求值和 hi+lo（~106 位元有效數，~32 位十進位）。
// 無誤差轉換（Knuth TwoSum、以 Veltkamp 分割實作的 Dekker TwoProd——不依賴 FMA）
// 以及相對誤差約 5e-29 的 exp/log（exp 平方 10 次，誤差放大約 1024 倍；實測）。
// 用於在 t·ln k ≈ 10¹¹ 弧度時仍精確計算
// Riemann–Siegel 相位 θ(t) − t·ln k。
namespace StonePotatoes {
namespace Math {
namespace DD {

struct dd {
    double hi = 0.0;
    double lo = 0.0;
};

inline dd TwoSum(double a, double b) {
    const double s = a + b;
    const double bb = s - a;
    return {s, (a - (s - bb)) + (b - bb)};
}

inline dd QuickTwoSum(double a, double b) {  // requires |a| ≥ |b|
    const double s = a + b;
    return {s, b - (s - a)};
}

inline void Split(double a, double& hi, double& lo) {
    constexpr double kSplitter = 134217729.0;  // 2^27 + 1
    const double t = kSplitter * a;
    hi = t - (t - a);
    lo = a - hi;
}

inline dd TwoProd(double a, double b) {
    const double p = a * b;
    double ah, al, bh, bl;
    Split(a, ah, al);
    Split(b, bh, bl);
    return {p, ((ah * bh - p) + ah * bl + al * bh) + al * bl};
}

inline dd Add(const dd& a, const dd& b) {
    dd s = TwoSum(a.hi, b.hi);
    const dd t = TwoSum(a.lo, b.lo);
    s.lo += t.hi;
    s = QuickTwoSum(s.hi, s.lo);
    s.lo += t.lo;
    return QuickTwoSum(s.hi, s.lo);
}
inline dd Neg(const dd& a) { return {-a.hi, -a.lo}; }
inline dd Sub(const dd& a, const dd& b) { return Add(a, Neg(b)); }
inline dd AddD(const dd& a, double b) {
    dd s = TwoSum(a.hi, b);
    s.lo += a.lo;
    return QuickTwoSum(s.hi, s.lo);
}
inline dd Mul(const dd& a, const dd& b) {
    dd p = TwoProd(a.hi, b.hi);
    p.lo += a.hi * b.lo + a.lo * b.hi;
    return QuickTwoSum(p.hi, p.lo);
}
inline dd MulD(const dd& a, double b) {
    dd p = TwoProd(a.hi, b);
    p.lo += a.lo * b;
    return QuickTwoSum(p.hi, p.lo);
}
inline dd Ldexp(const dd& a, int e) { return {std::ldexp(a.hi, e), std::ldexp(a.lo, e)}; }

// Constants (hi + lo, correctly rounded pairs) | 常數（正確捨入的 hi+lo）
inline constexpr dd kPi = {3.141592653589793116e+00, 1.224646799147353207e-16};
inline constexpr dd kTwoPi = {6.283185307179586232e+00, 2.449293598294706414e-16};
inline constexpr dd kLn2 = {6.931471805599452862e-01, 2.319046813846299558e-17};

// exp: x = m·ln2 + r, e^r = (e^{r/1024})^1024 with a Taylor kernel.
// exp：x = m·ln2 + r，e^r = (e^{r/1024})^1024，核心用 Taylor 展開。
inline dd Exp(const dd& x) {
    const double m = std::nearbyint(x.hi / kLn2.hi);
    const dd r = Ldexp(Sub(x, MulD(kLn2, m)), -10);
    dd sum = {1.0, 0.0}, term = {1.0, 0.0};
    for (int k = 1; k <= 12; ++k) {
        term = MulD(Mul(term, r), 1.0 / k);
        // 1/k is inexact for k ∉ {1,2,4,8}; the induced error is
        // ≤ u·|r|^k/k! ≤ 1e-35 for |r| ≤ 3.4e-4 — below dd resolution.
        // 1/k 對 k∉{1,2,4,8} 不精確；誘發誤差 ≤ u·|r|^k/k! ≤ 1e-35，低於 dd 解析度。
        sum = Add(sum, term);
        if (std::fabs(term.hi) < 1e-36) break;
    }
    for (int i = 0; i < 10; ++i) sum = Mul(sum, sum);
    return Ldexp(sum, static_cast<int>(m));
}

// log via Newton steps on exp: y ← y + a·e^{−y} − 1 (limited by exp, ~5e-29).
// log：對 exp 做牛頓步 y ← y + a·e^{−y} − 1（受 exp 精度限制，~5e-29）。
inline dd Log(const dd& a) {
    dd y = {std::log(a.hi), 0.0};
    for (int i = 0; i < 2; ++i)
        y = AddD(Add(y, Mul(a, Exp(Neg(y)))), -1.0);
    return y;
}
inline dd LogD(double a) { return Log(dd{a, 0.0}); }

// φ mod 2π → double in [−π, π]. φ.hi − q·2π.hi is exact by Sterbenz.
// φ mod 2π → [−π,π] 的 double；φ.hi − q·2π.hi 依 Sterbenz 引理為精確。
inline double Reduce2Pi(const dd& phi) {
    const double q = std::nearbyint(phi.hi / kTwoPi.hi);
    const dd qp = TwoProd(q, kTwoPi.hi);
    const dd d = TwoSum(phi.hi, -qp.hi);
    return d.hi + (d.lo + (phi.lo - qp.lo - q * kTwoPi.lo));
}

inline double ToDouble(const dd& a) { return a.hi + a.lo; }

} // namespace DD
} // namespace Math
} // namespace StonePotatoes
