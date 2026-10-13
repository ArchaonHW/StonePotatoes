#include "HighPrecisionZ.h"
#include "RiemannZeta.h"

#include <cmath>

namespace StonePotatoes {
namespace Math {
namespace Zeta {

namespace {
constexpr double kU = 1.1102230246251565e-16;  // 2^-53

const DD::dd& LogTwoPiDD() {
    static const DD::dd v = DD::Log(DD::kTwoPi);
    return v;
}
} // namespace

DD::dd ThetaDD(const DD::dd& t) {
    // θ = (t/2)·ln(t/2π) − t/2 − π/8 + 1/(48t) + 7/(5760t³) + 31/(80640t⁵)
    const DD::dd L = DD::Sub(DD::Log(t), LogTwoPiDD());
    const DD::dd h = DD::Ldexp(t, -1);  // exact | 精確
    DD::dd th = DD::Sub(DD::Mul(L, h), h);
    th = DD::Sub(th, DD::Ldexp(DD::kPi, -3));
    const double it = 1.0 / t.hi, it2 = it * it;
    th = DD::AddD(th, it * (1.0 / 48.0 + it2 * (7.0 / 5760.0 + it2 * (31.0 / 80640.0))));
    return th;
}

DD::dd GramPointDDExact(int64_t n) {
    const double nd = static_cast<double>(n);  // exact for |n| < 2^53
    DD::dd target = DD::TwoProd(nd, DD::kPi.hi);
    target = DD::AddD(target, nd * DD::kPi.lo);
    DD::dd g = {GramPoint(n), 0.0};
    for (int i = 0; i < 6; ++i) {
        const double diff = DD::ToDouble(DD::Sub(ThetaDD(g), target));
        const double step = diff / RSThetaPrime(g.hi);
        g = DD::AddD(g, -step);
        if (std::fabs(step) < 1e-28 * g.hi) break;
    }
    return g;
}

HighPrecisionZ::HighPrecisionZ(double tMax) : m_MaxT(tMax) {
    const size_t n = static_cast<size_t>(std::sqrt(tMax / kTwoPi)) + 2;
    m_LogK.resize(n + 1);
    m_InvSqrtK.resize(n + 1);
    for (size_t k = 1; k <= n; ++k) {
        m_LogK[k] = DD::LogD(static_cast<double>(k));
        m_InvSqrtK[k] = 1.0 / std::sqrt(static_cast<double>(k));
    }
}

double HighPrecisionZ::Z(const DD::dd& t) const {
    const double a = std::sqrt(DD::ToDouble(t) / kTwoPi);
    const int n = static_cast<int>(a);
    const double p = a - n;
    const DD::dd th = ThetaDD(t);
    double z = 0.0;
    for (int k = 1; k <= n; ++k) {
        const DD::dd& lk = m_LogK[static_cast<size_t>(k)];
        DD::dd tl = DD::TwoProd(t.hi, lk.hi);
        tl.lo += t.hi * lk.lo + t.lo * lk.hi;
        z += std::cos(DD::Reduce2Pi(DD::Sub(th, tl))) * m_InvSqrtK[static_cast<size_t>(k)];
    }
    const double sign = ((n - 1) % 2 == 0) ? 1.0 : -1.0;
    return 2.0 * z + sign * std::pow(a, -0.5) * (RSC0(p) + RSC1(p) / a);
}

double HighPrecisionZ::ErrorBound(double t) {
    const double a = std::sqrt(t / kTwoPi);
    const double sn = std::sqrt(a);
    // Last term: dd ln k / θ absolute error ≤ 1e-27 (measured 4.5e-28 for
    // exp∘log) times t, summed with weights Σ1/√k ≤ 2√N, doubled.
    // 末項：dd ln k / θ 的絕對誤差 ≤ 1e-27（exp∘log 實測 4.5e-28）乘 t，
    // 以權重 Σ1/√k ≤ 2√N 加總後乘 2。
    return 0.053 * std::pow(t, -1.25) + 2.0 * kU * (10.0 * sn + 2.0 * a * sn) +
           10.0 * kU * a + 4.0e-27 * t * sn;
}

double HardyZDerivativeBound(double t) {
    return 4.0 * std::log(t) * std::pow(t / kTwoPi, 0.25) + 4.0;
}

} // namespace Zeta
} // namespace Math
} // namespace StonePotatoes
