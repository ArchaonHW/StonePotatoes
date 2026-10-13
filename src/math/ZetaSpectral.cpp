#include "ZetaSpectral.h"
#include "RiemannZeta.h"

#include <algorithm>
#include <cmath>
#include <numeric>

namespace StonePotatoes {
namespace Math {
namespace Zeta {

namespace {

uint64_t SplitMix64(uint64_t x) {
    x += 0x9E3779B97F4A7C15ull;
    x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ull;
    x = (x ^ (x >> 27)) * 0x94D049BB133111EBull;
    return x ^ (x >> 31);
}

double ToUnit(uint64_t h) {
    return static_cast<double>(h >> 11) * (1.0 / 9007199254740992.0);
}

// Simpson average of f over [a,b] | f 在 [a,b] 的 Simpson 平均
template <class F>
double BinAverage(F f, double a, double b) {
    constexpr int n = 32;
    const double h = (b - a) / n;
    double s = f(a) + f(b);
    for (int i = 1; i < n; ++i) s += f(a + i * h) * (i % 2 ? 4.0 : 2.0);
    return s * h / 3.0 / (b - a);
}

} // namespace

double WignerSurmiseGUE(double s) {
    return (32.0 / (kPi * kPi)) * s * s * std::exp(-4.0 * s * s / kPi);
}

double WignerSurmiseGUECdf(double s) {
    if (s <= 0.0) return 0.0;
    return std::erf(2.0 * s / std::sqrt(kPi)) -
           (4.0 * s / kPi) * std::exp(-4.0 * s * s / kPi);
}

double MontgomeryR2(double u) {
    if (std::fabs(u) < 1e-12) return 0.0;
    const double x = std::sin(kPi * u) / (kPi * u);
    return 1.0 - x * x;
}

double ZeroSpectrum::Histogram::MaxAbsError() const {
    double e = 0.0;
    for (size_t i = 0; i < measured.size() && i < reference.size(); ++i)
        e = std::max(e, std::fabs(measured[i] - reference[i]));
    return e;
}

ZeroSpectrum::ZeroSpectrum(std::vector<double> gammas)
    : m_Gammas(std::move(gammas)) {
    std::sort(m_Gammas.begin(), m_Gammas.end());
}

ZeroSpectrum ZeroSpectrum::Compute(double tLo, double tHi, double step) {
    return ZeroSpectrum(
        ScanRoots([](double t) { return HardyZ(t); }, tLo, tHi, step));
}

std::vector<double> ZeroSpectrum::Unfolded() const {
    std::vector<double> xs;
    xs.reserve(m_Gammas.size());
    for (const double g : m_Gammas) xs.push_back(SmoothZeroCount(g));
    return xs;
}

std::vector<double> ZeroSpectrum::NormalizedSpacings() const {
    const std::vector<double> xs = Unfolded();
    std::vector<double> d;
    if (xs.size() < 2) return d;
    d.reserve(xs.size() - 1);
    for (size_t i = 1; i < xs.size(); ++i) d.push_back(xs[i] - xs[i - 1]);
    return d;
}

ZeroSpectrum::Histogram ZeroSpectrum::SpacingHistogram(double sMax,
                                                       int bins) const {
    Histogram h;
    h.binWidth = sMax / bins;
    h.measured.assign(static_cast<size_t>(bins), 0.0);
    h.reference.assign(static_cast<size_t>(bins), 0.0);
    const std::vector<double> d = NormalizedSpacings();
    for (const double s : d) {
        if (s < 0.0 || s >= sMax) continue;
        h.measured[static_cast<size_t>(s / h.binWidth)] += 1.0;
    }
    h.samples = d.size();
    for (int b = 0; b < bins; ++b) {
        const double a = b * h.binWidth, e = a + h.binWidth;
        if (h.samples)
            h.measured[static_cast<size_t>(b)] /=
                static_cast<double>(h.samples) * h.binWidth;
        h.reference[static_cast<size_t>(b)] =
            (WignerSurmiseGUECdf(e) - WignerSurmiseGUECdf(a)) / h.binWidth;
    }
    return h;
}

ZeroSpectrum::Histogram ZeroSpectrum::PairCorrelation(double uMax,
                                                      int bins) const {
    Histogram h;
    h.binWidth = uMax / bins;
    h.measured.assign(static_cast<size_t>(bins), 0.0);
    h.reference.assign(static_cast<size_t>(bins), 0.0);
    const std::vector<double> xs = Unfolded();
    for (size_t i = 0; i < xs.size(); ++i)
        for (size_t j = i + 1; j < xs.size(); ++j) {
            const double d = xs[j] - xs[i];
            if (d >= uMax) break;
            h.measured[static_cast<size_t>(d / h.binWidth)] += 1.0;
            ++h.samples;
        }
    for (int b = 0; b < bins; ++b) {
        const double a = b * h.binWidth;
        if (!xs.empty())
            h.measured[static_cast<size_t>(b)] /=
                static_cast<double>(xs.size()) * h.binWidth;
        h.reference[static_cast<size_t>(b)] =
            BinAverage(MontgomeryR2, a, a + h.binWidth);
    }
    return h;
}

namespace {
template <class Cdf>
double KSDistance(std::vector<double> d, Cdf cdf) {
    if (d.empty()) return 1.0;
    std::sort(d.begin(), d.end());
    const double n = static_cast<double>(d.size());
    double worst = 0.0;
    for (size_t i = 0; i < d.size(); ++i) {
        const double f = cdf(d[i]);
        worst = std::max({worst, std::fabs(f - i / n),
                          std::fabs((i + 1) / n - f)});
    }
    return worst;
}
} // namespace

double ZeroSpectrum::KSDistanceGUE() const {
    return KSDistance(NormalizedSpacings(), WignerSurmiseGUECdf);
}

double ZeroSpectrum::KSDistancePoisson() const {
    return KSDistance(NormalizedSpacings(),
                      [](double s) { return 1.0 - std::exp(-s); });
}

double ZeroSpectrum::MinNormalizedSpacing(double* atGamma) const {
    const std::vector<double> d = NormalizedSpacings();
    if (d.empty()) return 0.0;
    const size_t i = static_cast<size_t>(
        std::min_element(d.begin(), d.end()) - d.begin());
    if (atGamma) *atGamma = m_Gammas[i + 1];
    return d[i];
}

// ---------------------------------------------------------------------------
// SpectralPointSampler
// ---------------------------------------------------------------------------
SpectralPointSampler::SpectralPointSampler(const ZeroSpectrum& spectrum)
    : m_Spacings(spectrum.NormalizedSpacings()) {
    if (m_Spacings.empty()) return;
    const double mean =
        std::accumulate(m_Spacings.begin(), m_Spacings.end(), 0.0) /
        static_cast<double>(m_Spacings.size());
    for (double& s : m_Spacings) s /= mean;
}

std::vector<float> SpectralPointSampler::Sample1D(float length,
                                                  float meanSpacing,
                                                  uint32_t seed) const {
    std::vector<float> out;
    if (!IsValid() || length <= 0.0f || meanSpacing <= 0.0f) return out;
    const size_t n = m_Spacings.size();
    const uint64_t h = SplitMix64(seed);
    size_t idx = static_cast<size_t>(h % n);
    out.reserve(static_cast<size_t>(length / meanSpacing) + 2);
    double x = ToUnit(SplitMix64(h)) * m_Spacings[idx] * meanSpacing;
    while (x < length) {
        out.push_back(static_cast<float>(x));
        idx = (idx + 1) % n;
        x += m_Spacings[idx] * meanSpacing;
    }
    return out;
}

std::vector<SpectralPointSampler::Point2>
SpectralPointSampler::Sample2D(float width, float height, float meanSpacing,
                               uint32_t seed) const {
    std::vector<Point2> out;
    if (!IsValid() || width <= 0.0f || height <= 0.0f || meanSpacing <= 0.0f)
        return out;
    const double rowH = meanSpacing * 0.8660254037844386;
    const int rows = std::max(1, static_cast<int>(height / rowH));
    for (int r = 0; r < rows; ++r) {
        const uint64_t rowSeed =
            SplitMix64((static_cast<uint64_t>(seed) << 32) ^
                       static_cast<uint64_t>(r));
        const double y0 = (r + 0.5) * rowH;
        const std::vector<float> xs =
            Sample1D(width, meanSpacing, static_cast<uint32_t>(rowSeed));
        for (size_t i = 0; i < xs.size(); ++i) {
            // Bounded ±¼ row jitter breaks visible row lines without
            // re-introducing clumping.
            // 有界 ±¼ 列距抖動：打散列線，但不重新引入群聚。
            const double j =
                (ToUnit(SplitMix64(rowSeed ^ (i * 0x9E37ull))) - 0.5) * 0.5;
            const double y = std::clamp(y0 + j * rowH, 0.0,
                                        static_cast<double>(height));
            out.push_back({xs[i], static_cast<float>(y)});
        }
    }
    return out;
}

// ---------------------------------------------------------------------------
// SpectralScatter
// ---------------------------------------------------------------------------
SpectralScatter::SpectralScatter(const ZeroSpectrum& spectrum, const Config& cfg)
    : m_Cfg(cfg) {
    const std::vector<double> xs = spectrum.Unfolded();
    if (xs.size() < 8 || cfg.spacing <= 0.0f) return;
    const double n = static_cast<double>(xs.size());
    const double mean = (xs.back() - xs.front()) / (n - 1.0);
    m_U.reserve(xs.size());
    for (size_t j = 0; j < xs.size(); ++j) {
        m_U.push_back((xs[j] - xs.front()) / mean);
        m_MaxDev = std::max(m_MaxDev, std::fabs(m_U.back() - static_cast<double>(j)));
    }
    m_Cfg.rowJitter = std::clamp(m_Cfg.rowJitter, 0.0f, 0.5f);
}

double SpectralScatter::U(int64_t k) const {
    // Periodic extension with period n (the table's last gap closes at 1).
    // 以週期 n 延拓（表尾到下一週期的間距為 1）。
    const int64_t n = static_cast<int64_t>(m_U.size());
    int64_t q = k / n, r = k % n;
    if (r < 0) {
        r += n;
        --q;
    }
    return m_U[static_cast<size_t>(r)] + static_cast<double>(q) * static_cast<double>(n);
}

std::vector<SpectralScatter::Sample> SpectralScatter::Query(float x0, float y0,
                                                            float x1, float y1) const {
    std::vector<Sample> out;
    if (!IsValid() || x1 <= x0 || y1 <= y0) return out;
    const double s = m_Cfg.spacing;
    const double rowH = s * 0.8660254037844386;
    const int64_t n = static_cast<int64_t>(m_U.size());
    const int64_t margin = static_cast<int64_t>(std::ceil(2.0 * m_MaxDev)) + 2;
    const int64_t rLo = static_cast<int64_t>(std::floor(y0 / rowH)) - 1;
    const int64_t rHi = static_cast<int64_t>(std::floor(y1 / rowH)) + 1;
    for (int64_t r = rLo; r <= rHi; ++r) {
        const uint64_t rs = SplitMix64((static_cast<uint64_t>(m_Cfg.seed) << 32) ^
                                       static_cast<uint64_t>(r));
        const int64_t o = static_cast<int64_t>(rs % static_cast<uint64_t>(n));
        const double shift = ToUnit(SplitMix64(rs));
        const double base = U(o);
        int64_t i = static_cast<int64_t>(std::floor(x0 / s - shift)) - margin;
        for (;; ++i) {
            const float x = static_cast<float>((U(o + i) - base + shift) * s);
            if (x >= x1) break;
            if (x < x0) continue;
            const double j = (ToUnit(SplitMix64(rs ^ (static_cast<uint64_t>(i) *
                                                     0x9E3779B97F4A7C15ull))) -
                              0.5) * 2.0 * m_Cfg.rowJitter;
            const float y = static_cast<float>((static_cast<double>(r) + 0.5 + j) * rowH);
            if (y >= y0 && y < y1) out.push_back({x, y, r, i});
        }
    }
    return out;
}

// ---------------------------------------------------------------------------
// SpectralNoise
// ---------------------------------------------------------------------------
SpectralNoise::SpectralNoise(const ZeroSpectrum& spectrum, uint32_t seed,
                             int terms, double baseFrequency) {
    const auto& g = spectrum.Gammas();
    const size_t k = std::min(g.size(), static_cast<size_t>(std::max(terms, 0)));
    if (k == 0 || g[0] <= 0.0) return;
    double norm = 0.0;
    uint64_t h = SplitMix64(seed);
    for (size_t i = 0; i < k; ++i) {
        const double f = baseFrequency * g[i] / g[0];
        m_Freq.push_back(f);
        m_Amp.push_back(1.0 / (g[i] / g[0])); // pink-ish falloff | 近粉紅衰減
        norm += m_Amp.back() * m_Amp.back();
        h = SplitMix64(h);
        m_Phase.push_back(kTwoPi * ToUnit(h));
        h = SplitMix64(h);
        const double ang = kTwoPi * ToUnit(h);
        m_DirX.push_back(std::cos(ang));
        m_DirY.push_back(std::sin(ang));
    }
    // Σa²/2 = 1 → unit variance under independent phases.
    // Σa²/2=1 → 獨立相位下為單位變異。
    const double scale = std::sqrt(2.0 / norm);
    for (double& a : m_Amp) a *= scale;
}

double SpectralNoise::Evaluate(double x) const {
    double s = 0.0;
    for (size_t i = 0; i < m_Freq.size(); ++i)
        s += m_Amp[i] * std::cos(m_Freq[i] * x + m_Phase[i]);
    return s;
}

double SpectralNoise::Evaluate(double x, double y) const {
    double s = 0.0;
    for (size_t i = 0; i < m_Freq.size(); ++i)
        s += m_Amp[i] *
             std::cos(m_Freq[i] * (x * m_DirX[i] + y * m_DirY[i]) + m_Phase[i]);
    return s;
}

} // namespace Zeta
} // namespace Math
} // namespace StonePotatoes
