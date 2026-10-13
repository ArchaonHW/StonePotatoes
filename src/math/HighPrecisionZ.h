#pragma once

#include "DoubleDouble.h"
#include <cstdint>
#include <vector>

// HighPrecisionZ — Riemann–Siegel Hardy Z(t) with double-double phases.
// In plain double, each phase θ(t) − t·ln k (≈ t·ln t, ~10¹¹ rad at t≈3e9)
// loses ~u·t·ln t, so the rigorous error bound grows to ~2e-2 and certainty
// collapses near 10¹⁰ zeros. Here ln k (table), θ(t) and the reduction
// mod 2π are double-double; only cos(r), r ∈ [−π, π], runs in double, so the
// bound becomes Gabcke + O(u·N^{3/2}) ≈ 1e-9 at t ≈ 3e9.
// t itself may be double-double too: Gram points at t ≈ 10¹¹ are not
// representable to better than ulp(t) ≈ 4e-6 in double, which |Z′|·ulp
// would turn into a sign margin larger than |Z(g)| itself.
// HighPrecisionZ — 相位採 double-double 的 Riemann–Siegel Hardy Z(t)。
// 純 double 時每個相位 θ(t)−t·ln k（≈ t·ln t，t≈3e9 時 ~10¹¹ 弧度）損失
// ~u·t·ln t，嚴格誤差界升至 ~2e-2，約 10¹⁰ 個零點附近確定性崩潰。
// 這裡 ln k（查表）、θ(t) 與 mod 2π 約化皆為 double-double，僅 cos(r)
// （r∈[−π,π]）用 double，誤差界降為 Gabcke + O(u·N^{3/2})，t≈3e9 時約 1e-9。
// t 本身也可為 double-double：t≈10¹¹ 的 Gram 點在 double 中只能表示到
// ulp(t)≈4e-6，經 |Z′|·ulp 換算的符號餘裕會大於 |Z(g)| 本身。
namespace StonePotatoes {
namespace Math {
namespace Zeta {

// θ(t) in double-double (asymptotic series through t⁻⁵).
// θ(t) 的 double-double 版本（漸近級數至 t⁻⁵）。
DD::dd ThetaDD(const DD::dd& t);
inline DD::dd ThetaDD(double t) { return ThetaDD(DD::dd{t, 0.0}); }

// Gram point g_n solved in double-double (|error| ≲ 1e-27·g).
// 以 double-double 求解 Gram 點 g_n（誤差 ≲ 1e-27·g）。
DD::dd GramPointDDExact(int64_t n);
inline double GramPointDD(int64_t n) { return DD::ToDouble(GramPointDDExact(n)); }

class HighPrecisionZ {
public:
    // Builds ln k / k^{-1/2} tables for all t ≤ tMax.
    // 建立 t ≤ tMax 所需的 ln k / k^{-1/2} 表。
    explicit HighPrecisionZ(double tMax);

    double MaxT() const { return m_MaxT; }

    // Requires 200 ≤ t ≤ MaxT(). | 需 200 ≤ t ≤ MaxT()。
    double Z(const DD::dd& t) const;
    double Z(double t) const { return Z(DD::dd{t, 0.0}); }

    // Bound: Gabcke 0.053·t^{-5/4} + floating error of the dd-phase sum
    // 2u·(10√N + 2N√N) + remainder-term error 10u·a + dd ln/θ error.
    // 誤差界：Gabcke 0.053·t^{-5/4} + dd 相位求和浮點誤差 2u·(10√N + 2N√N)
    // + 餘項計算誤差 10u·a + dd 對數/θ 誤差。
    static double ErrorBound(double t);

    // Read-only table access for GPU upload (index k = 1..N).
    // 供 GPU 上傳用的唯讀表存取（索引 k = 1..N）。
    const std::vector<DD::dd>& LogK() const { return m_LogK; }
    const std::vector<double>& InvSqrtK() const { return m_InvSqrtK; }

private:
    double m_MaxT;
    std::vector<DD::dd> m_LogK;      // ln k, k ≥ 1 (index k) | 索引 k
    std::vector<double> m_InvSqrtK;  // k^{-1/2}
};

// Max |Z′| over a tiny interval near t (crude: 4·ln t·√N + 4) — used to turn
// a Gram-point location error δ into a sign margin |Z′|·δ.
// t 附近 |Z′| 粗略上界（4·ln t·√N + 4）——把 Gram 點位置誤差 δ 換算為符號餘裕 |Z′|·δ。
double HardyZDerivativeBound(double t);

} // namespace Zeta
} // namespace Math
} // namespace StonePotatoes
