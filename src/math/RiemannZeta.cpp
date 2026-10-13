#include "RiemannZeta.h"

namespace StonePotatoes {
namespace Math {
namespace Zeta {

double PsiChebyshev(double x) {
    const int n = static_cast<int>(x);
    if (n < 2) return 0.0;
    std::vector<char> composite(static_cast<size_t>(n) + 1, 0);
    double psi = 0.0;
    for (int p = 2; p <= n; ++p) {
        if (composite[static_cast<size_t>(p)]) continue;
        const double lp = std::log(static_cast<double>(p));
        for (long long pk = p; pk <= n; pk *= p) psi += lp;
        for (long long m = 2LL * p; m <= n; m += p)
            composite[static_cast<size_t>(m)] = 1;
    }
    return psi;
}

double PsiExplicit(double x, const std::vector<double>& gammas, double T) {
    double s = x - std::log(kTwoPi) - 0.5 * std::log(1.0 - 1.0 / (x * x));
    for (const double g : gammas) {
        if (g >= T) break;
        const double w = 1.0 - g / T;
        const C rho(0.5, g);
        s -= 2.0 * w * (std::pow(C(x, 0.0), rho) / rho).real();
    }
    return s;
}

std::vector<int8_t> MobiusSieve(uint32_t n) {
    std::vector<int8_t> mu(static_cast<size_t>(n) + 1, 1);
    std::vector<char> comp(static_cast<size_t>(n) + 1, 0);
    std::vector<uint32_t> primes;
    mu[0] = 0;
    for (uint32_t i = 2; i <= n; ++i) {
        if (!comp[i]) {
            primes.push_back(i);
            mu[i] = -1;
        }
        for (const uint32_t p : primes) {
            const uint64_t m = static_cast<uint64_t>(i) * p;
            if (m > n) break;
            comp[static_cast<size_t>(m)] = 1;
            if (i % p == 0) {
                mu[static_cast<size_t>(m)] = 0;
                break;
            }
            mu[static_cast<size_t>(m)] = static_cast<int8_t>(-mu[i]);
        }
    }
    return mu;
}

double MertensMaxRatio(uint32_t n, uint32_t* argMax) {
    const std::vector<int8_t> mu = MobiusSieve(n);
    int64_t m = n >= 1 ? 1 : 0;
    double worst = 0.0;
    uint32_t at = 0;
    for (uint32_t x = 2; x <= n; ++x) {
        m += mu[x];
        const double r = std::fabs(static_cast<double>(m)) /
                         std::sqrt(static_cast<double>(x));
        if (r > worst) {
            worst = r;
            at = x;
        }
    }
    if (argMax) *argMax = at;
    return worst;
}

double RobinMaxRatio(uint32_t n, uint32_t* argMax) {
    if (n < 5041) {
        if (argMax) *argMax = 0;
        return 0.0;
    }
    std::vector<uint64_t> sigma(static_cast<size_t>(n) + 1, 0);
    for (uint32_t d = 1; d <= n; ++d)
        for (uint32_t m = d; m <= n; m += d) sigma[m] += d;
    const double eG = std::exp(kEulerGamma);
    double worst = 0.0;
    uint32_t at = 0;
    for (uint32_t x = 5041; x <= n; ++x) {
        const double xd = static_cast<double>(x);
        const double r =
            static_cast<double>(sigma[x]) / (eG * xd * std::log(std::log(xd)));
        if (r > worst) {
            worst = r;
            at = x;
        }
    }
    if (argMax) *argMax = at;
    return worst;
}

double LiLambdaTruncated(int n, const std::vector<double>& gammas) {
    // On the critical line |1−1/ρ| = 1, so each conjugate pair contributes
    // 2(1 − cos nφ) with φ = arg((ρ−1)/ρ) — non-negative term by term.
    // 臨界線上 |1−1/ρ|=1，每對共軛零點貢獻 2(1−cos nφ)，逐項非負。
    double sum = 0.0;
    for (const double g : gammas) {
        const C rho(0.5, g);
        const double phi = std::arg((rho - C(1.0, 0.0)) / rho);
        sum += 2.0 * (1.0 - std::cos(n * phi));
    }
    return sum;
}

} // namespace Zeta
} // namespace Math
} // namespace StonePotatoes
