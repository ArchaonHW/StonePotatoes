#include "ZetaCompute.h"
#include "IQGPURuntime.h"
#include "../math/RiemannZeta.h"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstring>

namespace StonePotatoes {
namespace IQGPU {

using namespace Math::Zeta;

namespace {

double MsSince(std::chrono::steady_clock::time_point t0) {
    return std::chrono::duration<double, std::milli>(
               std::chrono::steady_clock::now() - t0)
        .count();
}

std::string Fmt(const char* fmt, double v) {
    char buf[64];
    std::snprintf(buf, sizeof(buf), fmt, v);
    return buf;
}

struct IntervalCount {
    uint32_t zeros = 0;
    uint32_t uncertain = 0;   // samples left inside the error band | 誤差帶內樣本
    uint32_t samples = 0;
    double minRatio = 1e300;  // min |Z|/bound | 最小餘裕倍數
};

// Margin-aware variant: an interior sample whose |Z| is within the HardyZ
// error bound is nudged (±h/4, ±h/2) until its sign is certain.
// 餘裕感知版本：內部樣本若 |Z| 落在誤差界內，微移（±h/4、±h/2）直到符號確定。
IntervalCount CountSignChangesCertain(double a, double b, double za, double zb,
                                      uint32_t samples) {
    IntervalCount r;
    const double h = (b - a) / samples;
    double prev = za;
    for (uint32_t j = 1; j <= samples; ++j) {
        double z = zb;
        if (j < samples) {
            const double t = a + h * j;
            z = HardyZ(t);
            ++r.samples;
            static constexpr double kNudge[4] = {0.25, -0.25, 0.5, -0.5};
            for (int k = 0; k < 4 && std::fabs(z) <= HardyZErrorBound(t); ++k) {
                z = HardyZ(t + kNudge[k] * h);
                ++r.samples;
            }
            const double ratio = std::fabs(z) / HardyZErrorBound(t);
            if (ratio <= 1.0) ++r.uncertain;
            r.minRatio = std::min(r.minRatio, ratio);
        }
        if (prev * z < 0.0) ++r.zeros;
        prev = z;
    }
    return r;
}

} // namespace

ZetaCompute::ZetaCompute(IQGPURuntime& runtime) : m_Runtime(runtime) {
    if (!m_Runtime.IsInitialized()) m_Runtime.Initialize();
}

std::vector<double> ZetaCompute::EvaluateZ(const std::vector<double>& ts,
                                           GPU::CPUBackend::Stats* stats) {
    std::vector<double> out(ts.size(), 0.0);
    auto s = m_Runtime.GetBackend().ParallelFor(
        static_cast<uint32_t>(ts.size()),
        [&](uint32_t i) { out[i] = HardyZ(ts[i]); }, 64);
    if (stats) *stats = s;
    return out;
}

ZeroSpectrum ZetaCompute::FindZeros(double tLo, double tHi, double step,
                                    GPU::CPUBackend::Stats* stats) {
    if (tHi <= tLo || step <= 0.0) return ZeroSpectrum();
    // Over-partition so uneven EM/RS cost (EM ~ t, RS ~ √t) load-balances.
    // 過度切分，使 EM/RS 成本不均時仍能負載平衡。
    const uint32_t segs = std::max<uint32_t>(
        1, std::min<uint32_t>(m_Runtime.GetBackend().WorkerCount() * 16,
                              static_cast<uint32_t>((tHi - tLo) / step / 8)));
    const double len = (tHi - tLo) / segs;
    const double segStep = len / std::max(1.0, std::ceil(len / step));
    std::vector<std::vector<double>> parts(segs);
    auto s = m_Runtime.GetBackend().ParallelFor(
        segs,
        [&](uint32_t i) {
            const double a = tLo + len * i;
            const double b = i + 1 == segs ? tHi : a + len;
            parts[i] = ScanRoots([](double t) { return HardyZ(t); }, a, b,
                                 segStep);
        },
        1);
    std::vector<double> roots;
    for (auto& p : parts) roots.insert(roots.end(), p.begin(), p.end());
    if (stats) *stats = s;
    return ZeroSpectrum(std::move(roots));
}

ZetaCompute::GramReport ZetaCompute::VerifyCriticalLine(
    int64_t nFirst, int64_t nLast, uint32_t samplesPerInterval) {
    const auto t0 = std::chrono::steady_clock::now();
    GramReport r;
    nFirst = std::max<int64_t>(nFirst, -1);
    if (nLast <= nFirst + 1 || samplesPerInterval == 0) return r;
    auto& be = m_Runtime.GetBackend();

    const size_t count = static_cast<size_t>(nLast - nFirst + 1);
    std::vector<double> g(count), zg(count);
    be.ParallelFor(static_cast<uint32_t>(count), [&](uint32_t i) {
        g[i] = GramPoint(nFirst + i);
        zg[i] = HardyZ(g[i]);
    }, 64);

    // Gram's law: (−1)^n Z(g_n) > 0 marks a "good" Gram point.
    // Gram 定律：(−1)^n Z(g_n) > 0 為「好」Gram 點。
    std::vector<char> good(count);
    for (size_t i = 0; i < count; ++i) {
        const int64_t n = nFirst + static_cast<int64_t>(i);
        good[i] = ((n % 2 == 0) ? zg[i] : -zg[i]) > 0.0;
    }

    std::vector<IntervalCount> iv(count - 1);
    be.ParallelFor(static_cast<uint32_t>(count - 1), [&](uint32_t i) {
        iv[i] = CountSignChangesCertain(g[i], g[i + 1], zg[i], zg[i + 1],
                                        samplesPerInterval);
    }, 16);

    size_t a = 0, b = count - 1;
    while (a < count && !good[a]) ++a;
    while (b > a && !good[b]) --b;
    if (a >= b) {
        r.ms = MsSince(t0);
        return r;
    }
    r.firstGram = nFirst + static_cast<int64_t>(a);
    r.lastGram = nFirst + static_cast<int64_t>(b);
    r.tLo = g[a];
    r.tHi = g[b];
    r.gramPoints = b - a + 1;

    // Rosser blocks: between consecutive good Gram points g_j < g_k there
    // must be exactly k − j zeros (assuming S(g_j) = S(g_k) = 0).
    // Rosser 區塊：相鄰好 Gram 點 g_j<g_k 之間應恰有 k−j 個零點。
    struct Block {
        size_t start;
        bool ok;
    };
    std::vector<Block> blocks;
    for (size_t j = a; j < b;) {
        size_t k = j + 1;
        while (!good[k]) ++k;
        const uint64_t expected = k - j;
        uint64_t found = 0;
        for (size_t i = j; i < k; ++i) found += iv[i].zeros;
        // Missing zeros come in pairs (endpoint signs fix parity) — refine.
        // 缺失零點必成對（端點符號固定奇偶）——細分重掃。
        for (uint32_t sres = samplesPerInterval * 8;
             found < expected && sres <= samplesPerInterval * 4096;
             sres *= 8) {
            found = 0;
            for (size_t i = j; i < k; ++i) {
                iv[i] = CountSignChangesCertain(g[i], g[i + 1], zg[i],
                                                zg[i + 1], sres);
                found += iv[i].zeros;
            }
            r.refinedIntervals += expected;
        }
        const bool ok = found == expected;
        if (!ok) r.unresolvedBlocks.push_back(nFirst + static_cast<int64_t>(j));
        blocks.push_back({j, ok});
        if (expected > 1) ++r.rosserBlocks;
        r.zerosExpected += expected;
        r.zerosFound += found;
        j = k;
    }
    r.minMarginRatio = 1e300;
    for (size_t i = a; i < b; ++i) {
        r.samplesEvaluated += iv[i].samples;
        r.uncertainSigns += iv[i].uncertain;
        r.minMarginRatio = std::min(r.minMarginRatio, iv[i].minRatio);
    }
    for (size_t i = a; i <= b; ++i) {
        const double ratio = std::fabs(zg[i]) / HardyZErrorBound(g[i]);
        r.minMarginRatio = std::min(r.minMarginRatio, ratio);
        if (ratio <= 1.0) ++r.uncertainSigns;  // Gram point sign unsure
        if (!good[i]) {
            ++r.gramLawFailures;
            if (r.firstFailures.size() < 16)
                r.firstFailures.push_back(nFirst + static_cast<int64_t>(i));
        }
    }
    r.verified = r.unresolvedBlocks.empty() && r.zerosFound == r.zerosExpected;

    // Turing/Brent: the last K Rosser-satisfying blocks bound N(g_n) from
    // above; the zeros counted on the line from g_{−1} (N = 0) bound it from
    // below → N(g_n) = n + 1 exactly.
    // Turing/Brent：最後 K 個滿足 Rosser 的區塊給出 N(g_n) 上界；
    // 從 g_{−1}（N=0）起線上計得的零點給出下界 → N(g_n)=n+1 精確成立。
    const int64_t K = TuringBlocksRequired(g[b]);
    r.turingBlocksUsed = K;
    if (r.verified && r.uncertainSigns == 0 && r.firstGram == -1 &&
        static_cast<int64_t>(blocks.size()) > K) {
        const size_t idx = blocks[blocks.size() - static_cast<size_t>(K)].start;
        r.turingCertified = true;
        r.certifiedGram = nFirst + static_cast<int64_t>(idx);
        r.certifiedHeight = g[idx];
        r.certifiedZeros = static_cast<uint64_t>(r.certifiedGram + 1);
    }
    r.ms = MsSince(t0);
    return r;
}

std::string ZetaCompute::AuditReport::ToString() const {
    std::string out;
    for (const auto& it : items)
        out += it.name + "=" + it.value + (it.pass ? " pass=1\n" : " pass=0\n");
    char buf[128];
    std::snprintf(buf, sizeof(buf),
                  "audit.passed=%d/%d ms=%.1f finite_verification=1 proof=0",
                  passed, total, ms);
    return out + buf;
}

ZetaCompute::AuditReport ZetaCompute::RunAudit(const AuditConfig& cfgIn) {
    const auto t0 = std::chrono::steady_clock::now();
    AuditConfig cfg = cfgIn;
    cfg.tMax = std::clamp(cfg.tMax, 20.0, 400.0);
    cfg.spectrumTMax = std::clamp(cfg.spectrumTMax, cfg.tMax, 1e5);
    cfg.xMax = std::max<uint32_t>(cfg.xMax, 5041);
    cfg.debruijnLambda = std::clamp(cfg.debruijnLambda, 0.0, 0.5);
    auto& be = m_Runtime.GetBackend();

    AuditReport rep;
    auto emit = [&](std::string name, std::string value, bool ok) {
        rep.items.push_back({std::move(name), std::move(value), ok});
        ++rep.total;
        if (ok) ++rep.passed;
    };

    // 1) Zeros on the line up to spectrumTMax (parallel scan).
    // 1) 線上零點掃描至 spectrumTMax（平行）。
    const ZeroSpectrum spec = FindZeros(10.0, cfg.spectrumTMax + 5.0, 0.02);
    const auto& gam = spec.Gammas();

    // Pick the winding top edge halfway between two zeros near tMax so the
    // contour never grazes a zero.
    // 輻角矩形頂邊取 tMax 附近兩零點中點，避免路徑擦過零點。
    size_t below = static_cast<size_t>(
        std::upper_bound(gam.begin(), gam.end(), cfg.tMax) - gam.begin());
    double top = cfg.tMax;
    if (below > 0 && below < gam.size())
        top = 0.5 * (gam[below - 1] + gam[below]);
    const double smooth = SmoothZeroCount(top);
    emit("zeros.on_line", std::to_string(below) + " (N0(T)=" +
                              Fmt("%.2f", smooth) + " T=" + Fmt("%.3f", top) + ")",
         below > 0 && std::fabs(static_cast<double>(below) - smooth) < 2.5);

    // 2) Argument principle over σ ∈ [−0.5, 1.5]: counts ALL zeros in the
    //    strip; equality with the on-line count ⇒ none off the line.
    //    Contour samples evaluated in parallel, unwrapped sequentially.
    // 2) 輻角原理覆蓋整條帶 σ∈[−0.5,1.5]：與線上計數相等 ⇒ 無離線零點。
    //    路徑取樣平行求值，相位展開循序累加。
    {
        const double sLo = -0.5, sHi = 1.5, tLo = 10.0, h = 0.01;
        std::vector<C> pts;
        const int sN = static_cast<int>((sHi - sLo) / h);
        const int tN = static_cast<int>((top - tLo) / h);
        for (int i = 0; i <= sN; ++i) pts.emplace_back(sLo + (sHi - sLo) * i / sN, tLo);
        for (int i = 1; i <= tN; ++i) pts.emplace_back(sHi, tLo + (top - tLo) * i / tN);
        for (int i = 1; i <= sN; ++i) pts.emplace_back(sHi - (sHi - sLo) * i / sN, top);
        for (int i = 1; i <= tN; ++i) pts.emplace_back(sLo, top - (top - tLo) * i / tN);
        std::vector<double> arg(pts.size());
        be.ParallelFor(static_cast<uint32_t>(pts.size()), [&](uint32_t i) {
            arg[i] = std::arg(ZetaEM(pts[i]));
        }, 64);
        double wind = 0.0;
        for (size_t i = 1; i < arg.size(); ++i) {
            double d = arg[i] - arg[i - 1];
            while (d > kPi) d -= kTwoPi;
            while (d < -kPi) d += kTwoPi;
            wind += d;
        }
        const long w = std::lround(wind / kTwoPi);
        emit("winding.strip_count", std::to_string(w),
             w == static_cast<long>(below));
    }

    // 3) Gram / Rosser-block critical-line verification.
    // 3) Gram / Rosser 區塊臨界線驗證。
    {
        const GramReport gr = VerifyCriticalLine(-1, cfg.gramLast);
        char buf[256];
        std::snprintf(buf, sizeof(buf),
                      "%llu/%llu zeros to t=%.1f bad_gram=%llu rosser_blocks=%llu",
                      static_cast<unsigned long long>(gr.zerosFound),
                      static_cast<unsigned long long>(gr.zerosExpected), gr.tHi,
                      static_cast<unsigned long long>(gr.gramLawFailures),
                      static_cast<unsigned long long>(gr.rosserBlocks));
        emit("gram.critical_line", buf, gr.verified);
        std::snprintf(buf, sizeof(buf),
                      "N(%.2f)=%llu exact (K=%lld) uncertain_signs=%llu min_margin=%.1fx",
                      gr.certifiedHeight,
                      static_cast<unsigned long long>(gr.certifiedZeros),
                      static_cast<long long>(gr.turingBlocksUsed),
                      static_cast<unsigned long long>(gr.uncertainSigns),
                      gr.minMarginRatio);
        emit("turing.certified", buf, gr.turingCertified);
    }

    // 4) Random-matrix fingerprint: GUE spacing (KS) + Montgomery repulsion.
    // 4) 隨機矩陣指紋：GUE 間距（KS）+ Montgomery 排斥。
    {
        const double ksG = spec.KSDistanceGUE(), ksP = spec.KSDistancePoisson();
        emit("gue.ks", Fmt("%.4f", ksG) + " (poisson " + Fmt("%.4f", ksP) + ")",
             ksG < ksP && ksG < 0.15);
        const auto pc = spec.PairCorrelation(3.0, 6);
        emit("montgomery.r2_bin0",
             Fmt("%.3f", pc.measured.empty() ? 1.0 : pc.measured[0]) +
                 " (GUE " + Fmt("%.3f", pc.reference.empty() ? 0.0 : pc.reference[0]) +
                 ", poisson 1)",
             !pc.measured.empty() && pc.measured[0] < 0.45);
    }

    // 5) Explicit formula: zeros reconstruct the prime staircase ψ(x).
    // 5) 顯式公式：零點重建質數階梯 ψ(x)。
    {
        const double xs[4] = {40.5, 61.7, 83.2, 99.3};
        double worst = 0.0;
        for (const double x : xs)
            worst = std::max(worst, std::fabs(PsiExplicit(x, gam, cfg.spectrumTMax) -
                                              PsiChebyshev(x)));
        // Fejér truncation error scales ~1/T (measured: T=200 → 2.0,
        // T=1000 → 0.79); tolerance follows the same scale.
        // Fejér 截斷誤差約隨 1/T 縮放（實測 T=200→2.0、T=1000→0.79），門檻同比例。
        const double tol = 1000.0 / cfg.spectrumTMax;
        emit("explicit.psi_max_err",
             Fmt("%.4f", worst) + " tol " + Fmt("%.3f", tol), worst < tol);
    }

    // 6) Mertens & Robin (arithmetic equivalents / consequences of RH).
    // 6) Mertens 與 Robin（RH 的算術等價式／推論）。
    {
        uint32_t at = 0;
        const double m = MertensMaxRatio(cfg.xMax, &at);
        emit("mertens.max_ratio", Fmt("%.4f", m) + " at x=" + std::to_string(at),
             m < 1.0);
        const double rb = RobinMaxRatio(cfg.xMax, &at);
        emit("robin.max_ratio", Fmt("%.6f", rb) + " at n=" + std::to_string(at),
             rb < 1.0);
    }

    // 7) Li λ₁: truncated sum must approach λ₁ = 1 + γ/2 − ½ln 4π from below,
    //    with tail ≈ (ln(T/2π)+1)/(2πT).
    // 7) Li λ₁：截斷和自下方逼近 λ₁ 精確值，尾項 ≈ (ln(T/2π)+1)/(2πT)。
    {
        const double exact = 1.0 + 0.5 * kEulerGamma - 0.5 * std::log(4.0 * kPi);
        std::vector<double> upto;
        for (const double gm : gam)
            if (gm < cfg.spectrumTMax) upto.push_back(gm);
        const double tr = LiLambdaTruncated(1, upto);
        const double T = cfg.spectrumTMax;
        const double tail = (std::log(T / kTwoPi) + 1.0) / (kTwoPi * T);
        emit("li.lambda1", Fmt("%.6f", tr) + " exact " + Fmt("%.6f", exact) +
                               " tail~" + Fmt("%.6f", tail),
             tr > 0.0 && tr <= exact && std::fabs(exact - tr) < 2.0 * tail);
    }

    // 8) de Bruijn–Newman: H_0 zeros are 2γ (independent kernel cross-check);
    //    H_λ zeros stay real in the window.
    // 8) de Bruijn–Newman：H_0 零點為 2γ（獨立核交叉驗證）；H_λ 零點在窗內維持實數。
    {
        const auto h0 = ScanRoots([](double x) { return HtFlow(x, 0.0); }, 4.0, 85.0, 0.05);
        const size_t expect = static_cast<size_t>(
            std::upper_bound(gam.begin(), gam.end(), 42.5) - gam.begin());
        emit("debruijn.h0_vs_2gamma",
             std::to_string(h0.size()) + "/" + std::to_string(expect),
             h0.size() == expect);
        const double lam = cfg.debruijnLambda;
        const auto hl = ScanRoots([lam](double x) { return HtFlow(x, lam); }, 4.0, 85.0, 0.05);
        emit("debruijn.lambda_real_zeros",
             std::to_string(hl.size()) + " (lambda=" + Fmt("%.2f", lam) + ")",
             hl.size() + 1 >= expect);
    }

    rep.ms = MsSince(t0);
    return rep;
}

ZetaCompute::AuditReport ZetaCompute::SelfTest() {
    const auto t0 = std::chrono::steady_clock::now();
    AuditReport rep;
    auto emit = [&](std::string name, std::string value, bool ok) {
        rep.items.push_back({std::move(name), std::move(value), ok});
        ++rep.total;
        if (ok) ++rep.passed;
    };

    // Known answers (Odlyzko tables; N(1000) = 649; first Gram failure 126).
    // 已知答案（Odlyzko 表；N(1000)=649；首個 Gram 失效 126）。
    static constexpr double kZeros[5] = {14.134725141734693, 21.022039638771555,
                                         25.010857580145688, 30.424876125859513,
                                         32.935061587739189};
    const ZeroSpectrum spec = FindZeros(10.0, 1000.0, 0.02);
    emit("golden.N1000", std::to_string(spec.Size()) + "/649", spec.Size() == 649);
    double zerr = 1.0;
    if (spec.Size() >= 5) {
        zerr = 0.0;
        for (int i = 0; i < 5; ++i)
            zerr = std::max(zerr, std::fabs(spec.Gammas()[i] - kZeros[i]));
    }
    emit("golden.odlyzko_max_err", Fmt("%.2e", zerr), zerr < 1e-6);

    const GramReport gr = VerifyCriticalLine(-1, 1000, 8);
    emit("golden.gram_first_failure",
         gr.firstFailures.empty() ? "none" : std::to_string(gr.firstFailures[0]),
         !gr.firstFailures.empty() && gr.firstFailures[0] == 126);
    emit("golden.turing_certified",
         std::to_string(gr.certifiedZeros) + " zeros to t=" +
             Fmt("%.2f", gr.certifiedHeight),
         gr.turingCertified && gr.certifiedZeros >= 990);

    // Determinism: the same Z(t) bits from 1 worker and from N workers —
    // required for lockstep networking and replay on any backend.
    // 確定性：1 worker 與 N worker 產生相同位元的 Z(t)——
    // 任何後端上的鎖步網路與重播皆需此性質。
    std::vector<double> ts(4096);
    for (size_t i = 0; i < ts.size(); ++i) ts[i] = 10.0 + 1.2207 * static_cast<double>(i);
    const std::vector<double> par = EvaluateZ(ts);
    GPU::CPUBackend single(1);
    std::vector<double> seq(ts.size());
    single.ParallelFor(static_cast<uint32_t>(ts.size()),
                       [&](uint32_t i) { seq[i] = HardyZ(ts[i]); }, 4096);
    const bool same = std::memcmp(par.data(), seq.data(), par.size() * sizeof(double)) == 0;
    emit("golden.bit_determinism",
         std::to_string(ts.size()) + " samples, " +
             std::to_string(m_Runtime.GetBackend().WorkerCount()) + " vs 1 workers",
         same);

    rep.ms = MsSince(t0);
    return rep;
}

// --- Static scan helpers | 靜態掃描輔助 ---

ZetaCompute::ScanResult ZetaCompute::FindZeros(GPU::CPUBackend& backend,
                                               double tLo, double tHi,
                                               double step) {
    ScanResult r;
    if (tHi <= tLo || step <= 0.0) return r;
    const auto t0 = std::chrono::steady_clock::now();
    const uint32_t segs = std::max<uint32_t>(
        1, std::min<uint32_t>(backend.WorkerCount() * 16,
                              static_cast<uint32_t>((tHi - tLo) / step / 8)));
    const double len = (tHi - tLo) / segs;
    const double segStep = len / std::max(1.0, std::ceil(len / step));
    std::vector<std::vector<double>> parts(segs);
    backend.ParallelFor(
        segs,
        [&](uint32_t i) {
            const double a = tLo + len * i;
            const double b = i + 1 == segs ? tHi : a + len;
            parts[i] = ScanRoots([](double t) { return HardyZ(t); }, a, b,
                                 segStep);
        },
        1);
    for (auto& p : parts) r.gammas.insert(r.gammas.end(), p.begin(), p.end());
    r.signChanges = static_cast<uint32_t>(r.gammas.size());
    r.samplesEvaluated =
        static_cast<uint32_t>((tHi - tLo) / step);
    r.scanMs = MsSince(t0);  // sampling + bisection share this pass
    r.refineMs = 0.0;        // refinement is fused into ScanRoots
    return r;
}

ZetaCompute::ScanResult ZetaCompute::FindZerosSequential(double tLo,
                                                       double tHi,
                                                       double step) {
    ScanResult r;
    if (tHi <= tLo || step <= 0.0) return r;
    const auto t0 = std::chrono::steady_clock::now();
    r.gammas = ScanRoots([](double t) { return HardyZ(t); }, tLo, tHi, step);
    r.signChanges = static_cast<uint32_t>(r.gammas.size());
    r.samplesEvaluated = static_cast<uint32_t>((tHi - tLo) / step);
    r.scanMs = MsSince(t0);
    return r;
}

} // namespace IQGPU
} // namespace StonePotatoes
