// StonePotatoes math + IQGPU module tests.
// StonePotatoes 數學 + IQGPU 模組測試。

#include "math/RiemannZeta.h"
#include "math/ZetaSpectral.h"
#include "math/HighPrecisionZ.h"
#include "iqgpu/ZetaVerificationJob.h"
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include "gpu/CPUBackend.h"
#include "gpu/ComputeGraph.h"
#include "gpu/Kernel.h"
#include "iqgpu/QuantumInspired.h"
#include "iqgpu/ZetaCompute.h"
#include "iqgpu/IQGPURuntime.h"
#include "world/procedural/ResourceScatter.h"

#include <algorithm>

#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>

using namespace StonePotatoes;

static int g_Passed = 0;
static int g_Failed = 0;

#define CHECK(cond)                                                        \
    do {                                                                   \
        if (cond) { ++g_Passed; }                                          \
        else {                                                             \
            ++g_Failed;                                                    \
            std::printf("[FAIL] %s:%d  %s\n", __FILE__, __LINE__, #cond);   \
        }                                                                  \
    } while (0)

static bool Near(double a, double b, double tol) {
    return std::fabs(a - b) < tol;
}

// --- RiemannZeta / HardyZ | 黎曼 ζ / HardyZ ---
void TestRiemannZeta() {
    // First three non-trivial zeros: 14.134725, 21.022040, 25.010858.
    // Hardy Z vanishes at each.
    CHECK(std::fabs(Math::Zeta::HardyZ(14.134725)) < 0.01);
    CHECK(std::fabs(Math::Zeta::HardyZ(21.022040)) < 0.01);
    CHECK(std::fabs(Math::Zeta::HardyZ(25.010858)) < 0.01);

    // Between zeros it must be clearly non-zero.
    CHECK(std::fabs(Math::Zeta::HardyZ(18.0)) > 0.5);

    // Smooth count N0(T)=θ(T)/π+1 vs true count: 10 zeros below 50, S small.
    CHECK(Near(Math::Zeta::SmoothZeroCount(50.0), 10.0, 1.0));
    // Trudgian S(T) bound: known N(100)=29, N(1000)=649 (Odlyzko tables).
    // Trudgian S(T) 界：已知 N(100)=29、N(1000)=649。
    CHECK(Math::Zeta::ZeroCountConsistent(100.0, 29));
    CHECK(Math::Zeta::ZeroCountConsistent(1000.0, 649));
    CHECK(!Math::Zeta::ZeroCountConsistent(1000.0, 640));
    CHECK(!Math::Zeta::ZeroCountConsistent(1000.0, 660));
    CHECK(!Math::Zeta::ZeroCountConsistent(1.0, 0));
    CHECK(Math::Zeta::ZeroCountErrorBound(1e6) > Math::Zeta::ZeroCountErrorBound(1e3));
    {
        // Cross-check against the scanner's own count up to t = 60 (13 zeros).
        // 與掃描器在 t ≤ 60 的實際零點數（13 個）交叉檢查。
        auto z = IQGPU::ZetaCompute::FindZerosSequential(10.0, 60.0, 0.05);
        CHECK(z.gammas.size() == 13);
        CHECK(Math::Zeta::ZeroCountConsistent(60.0, static_cast<int64_t>(z.gammas.size())));
    }

    // Gram points are real and increasing.
    const double g0 = Math::Zeta::GramPoint(0);
    const double g1 = Math::Zeta::GramPoint(1);
    CHECK(g0 > 0.0 && g1 > g0);
}

// --- ZeroSpectrum (scanned, small range) | 零點頻譜（小範圍掃描） ---
void TestZeroSpectrum() {
    auto spectrum = Math::Zeta::ZeroSpectrum::Compute(10.0, 60.0, 0.02);
    CHECK(spectrum.Size() >= 5);                  // ~7 zeros in [10,60]
    CHECK(Near(spectrum.Gammas().front(), 14.134725, 0.05));

    auto spacings = spectrum.NormalizedSpacings();
    CHECK(spacings.size() + 1 == spectrum.Size());
    for (double s : spacings) {
        CHECK(s > 0.0);                            // level repulsion: no 0-gap
    }

    // GUE is the right model: KS distance to GUE should beat Poisson.
    CHECK(spectrum.KSDistanceGUE() < spectrum.KSDistancePoisson());
}

// --- SpectralPointSampler | 頻譜取樣器 ---
void TestSpectralSampler() {
    auto spectrum = Math::Zeta::ZeroSpectrum::Compute(10.0, 60.0, 0.02);
    Math::Zeta::SpectralPointSampler sampler(spectrum);
    CHECK(sampler.IsValid());

    auto pts = sampler.Sample1D(200.0f, 5.0f, 12345);
    CHECK(pts.size() >= 30);                       // ~40 expected
    for (size_t i = 1; i < pts.size(); ++i) {
        CHECK(pts[i] > pts[i - 1]);                // strictly ordered
    }
    CHECK(pts.front() >= 0.0f && pts.back() < 200.0f);

    // Deterministic per seed.
    auto pts2 = sampler.Sample1D(200.0f, 5.0f, 12345);
    CHECK(pts == pts2);
}

// --- SpectralNoise | 頻譜雜訊 ---
void TestSpectralNoise() {
    auto spectrum = Math::Zeta::ZeroSpectrum::Compute(10.0, 60.0, 0.02);
    Math::Zeta::SpectralNoise noise(spectrum, 7, 16);
    CHECK(noise.IsValid());

    CHECK(Near(noise.Evaluate(3.25), noise.Evaluate(3.25), 1e-12));
    CHECK(!Near(noise.Evaluate(3.25), noise.Evaluate(7.99), 1e-9));

    double sum = 0.0;
    for (int i = 0; i < 100; ++i) sum += noise.Evaluate(i * 0.37);
    CHECK(std::fabs(sum) < 50.0); // roughly zero-mean
}

// --- CPUBackend dispatch | CPU 後端派發 ---
void TestCPUBackend() {
    GPU::CPUBackend backend(4);
    CHECK(backend.WorkerCount() == 4);

    constexpr uint32_t N = 10000;
    std::vector<float> data(N, 1.0f);
    GPU::Kernel k("inc", [](uint32_t gid, void* ud) {
        static_cast<float*>(ud)[gid] += 1.0f;
    });
    GPU::DispatchParams p;
    p.threadCount = N;
    p.userData = data.data();

    auto stats = backend.Dispatch(k, p);
    CHECK(stats.itemsProcessed == N);
    CHECK(stats.threadsUsed > 0);
    CHECK(data[0] == 2.0f && data[N - 1] == 2.0f);

    // Async dispatch
    std::vector<float> data2(N, 0.0f);
    GPU::DispatchParams p2;
    p2.threadCount = N;
    p2.userData = data2.data();
    auto fut = backend.DispatchAsync(k, p2);
    auto s2 = fut.get();
    CHECK(s2.itemsProcessed == N);
    CHECK(data2[N / 2] == 1.0f);

    // ParallelFor — every index visited exactly once.
    std::atomic<uint32_t> visited{0};
    std::vector<uint32_t> hits(N, 0);
    auto pf = backend.ParallelFor(N, [&](uint32_t i) {
        ++hits[i];
        visited.fetch_add(1, std::memory_order_relaxed);
    }, 64);
    CHECK(pf.itemsProcessed == N);
    CHECK(visited.load() == N);
    CHECK(hits[0] == 1 && hits[N - 1] == 1);
}

// --- ComputeGraph | 計算圖 ---
void TestComputeGraph() {
    GPU::CPUBackend backend(4);
    GPU::ComputeGraph graph;

    std::vector<int> order;
    auto a = GPU::ComputeNode::MakeTask("A", [&] { order.push_back(1); });
    auto b = GPU::ComputeNode::MakeTask("B", [&] { order.push_back(2); });
    auto c = GPU::ComputeNode::MakeTask("C", [&] { order.push_back(3); });
    b->AddDependency(a);
    c->AddDependency(b);
    graph.AddNode(a);
    graph.AddNode(b);
    graph.AddNode(c);

    auto r = graph.Execute(backend);
    CHECK(r.ok);
    CHECK(r.nodesExecuted == 3);
    CHECK(order == std::vector<int>({1, 2, 3}));

    // Cycle detection
    GPU::ComputeGraph cyc;
    auto x = GPU::ComputeNode::MakeTask("X", [] {});
    auto y = GPU::ComputeNode::MakeTask("Y", [] {});
    x->AddDependency(y);
    y->AddDependency(x);
    cyc.AddNode(x);
    cyc.AddNode(y);
    auto rc = cyc.Execute(backend);
    CHECK(!rc.ok || rc.cycleDetected);
}

// --- Quantum-inspired | 量子啟發 ---
void TestQuantumInspired() {
    // Convex bowl: unique global minimum at x_i = 2.
    auto energy = [](const std::vector<double>& x) {
        double e = 0;
        for (double v : x) e += (v - 2.0) * (v - 2.0);
        return e;
    };
    auto perturb = [](std::vector<double>& x, std::mt19937& rng) {
        std::uniform_int_distribution<size_t> pick(0, x.size() - 1);
        std::normal_distribution<double> n(0.0, 0.5);
        x[pick(rng)] += n(rng);
    };

    std::vector<double> init(4, 10.0);
    IQGPU::SimulatedAnnealer sa;
    auto r = sa.Optimize(init, energy, perturb, 5000, 5.0, 0.999, 42);
    CHECK(r.bestEnergy < energy(init));
    CHECK(r.improvement > 0.0);

    // Amplitude amplification on a marked set.
    IQGPU::AmplitudeAmplifier aa;
    auto rr = aa.Search(128, [](uint32_t i) { return i % 7 == 0 && i != 0; }, 6);
    CHECK(!rr.markedStates.empty());
    double marked = 0.0;
    for (uint32_t i = 0; i < 128; ++i) {
        if (i % 7 == 0 && i != 0) marked += rr.probabilities[i];
    }
    CHECK(marked > 0.5);   // well above uniform ~14%

    // ProbabilisticStateSpace normalization + deterministic-ish measure.
    IQGPU::ProbabilisticStateSpace pss(3, 11);
    pss.SetAmplitude(0, 1.0);
    pss.SetAmplitude(1, 1.0);
    pss.SetAmplitude(2, std::sqrt(2.0));
    pss.Normalize();
    double total = pss.GetProbability(0) + pss.GetProbability(1) +
                   pss.GetProbability(2);
    CHECK(Near(total, 1.0, 1e-9));
    CHECK(Near(pss.GetProbability(2), 0.5, 1e-9));
    for (int i = 0; i < 100; ++i) {
        CHECK(pss.Measure() < 3);
    }

    // A zero-amplitude state must never be selected, and measurement counts
    // should track the Born-rule probabilities for a fixed seeded sequence.
    IQGPU::ProbabilisticStateSpace sampled(3, 12345);
    sampled.SetAmplitude(0, 0.0);
    sampled.SetAmplitude(1, 1.0);
    sampled.SetAmplitude(2, 2.0);
    sampled.Normalize();
    CHECK(sampled.GetAmplitude(3) == 0.0);
    CHECK(sampled.GetProbability(3) == 0.0);

    constexpr int kMeasurementCount = 20000;
    int counts[3]{};
    for (int i = 0; i < kMeasurementCount; ++i) {
        const uint32_t state = sampled.Measure();
        CHECK(state != 0);
        CHECK(state < 3);
        if (state < 3) ++counts[state];
    }
    CHECK(Near(static_cast<double>(counts[1]) / kMeasurementCount,
               0.2, 0.015));
    CHECK(Near(static_cast<double>(counts[2]) / kMeasurementCount,
               0.8, 0.015));
}

// --- ZetaCompute (IQGPU parallel scan) | ζ 平行掃描 ---
void TestZetaCompute() {
    // Sequential reference over a small range.
    auto seq = IQGPU::ZetaCompute::FindZerosSequential(10.0, 60.0, 0.05);
    CHECK(seq.gammas.size() >= 3);
    CHECK(Near(seq.gammas[0], 14.134725, 0.1));

    // Parallel scan must agree.
    GPU::CPUBackend backend(4);
    auto par = IQGPU::ZetaCompute::FindZeros(backend, 10.0, 60.0, 0.05);
    CHECK(par.gammas.size() == seq.gammas.size());
    CHECK(Near(par.gammas[0], seq.gammas[0], 1e-6));
}

// --- RH finite verification (PotatoEngine port) | RH 有限驗證 ---
void TestRHVerification() {
    using namespace Math::Zeta;

    // C0 Taylor series must match the closed form away from the removable
    // singularities at p = 1/4, 3/4.
    // C0 級數須與閉式一致（避開 p=1/4、3/4 可去奇點）。
    double c0err = 0.0;
    for (double p = 0.0; p <= 1.0; p += 0.001)
        if (std::fabs(std::cos(kTwoPi * p)) > 1e-2)
            c0err = std::max(c0err, std::fabs(RSC0(p) - RSC0Closed(p)));
    CHECK(c0err < 1e-10);

    // Riemann–Siegel + C0 + C1 vs Euler–Maclaurin (measured ~7e-5).
    // RS 修正版對照 EM（實測 ~7e-5）。
    double rsErr = 0.0;
    for (double t = 400.0; t <= 1200.0; t += 1.37)
        rsErr = std::max(rsErr, std::fabs(RSZCorrected(t) - RotZeta(t)));
    CHECK(rsErr < 5e-4);

    // First 10 zeros (Odlyzko tables) to high precision.
    // 前 10 個零點（Odlyzko 表）高精度比對。
    const double known[10] = {
        14.134725141734693, 21.022039638771555, 25.010857580145688,
        30.424876125859513, 32.935061587739189, 37.586178158825671,
        40.918719012147495, 43.327073280914999, 48.005150881167159,
        49.773832477672302};
    auto spec = ZeroSpectrum::Compute(10.0, 50.0, 0.02);
    CHECK(spec.Size() == 10);
    for (size_t i = 0; i < 10 && i < spec.Size(); ++i)
        CHECK(Near(spec.Gammas()[i], known[i], 1e-6));

    CHECK(Near(GramPoint(0), 17.8455995, 1e-6));

    IQGPU::IQGPURuntime rt;
    rt.Initialize();
    IQGPU::ZetaCompute zc(rt);

    // N(1000) = 649 zeros (classical value).
    CHECK(zc.FindZeros(10.0, 1000.0, 0.02).Size() == 649);

    // Gram's law first fails at n = 126; Rosser blocks still account for
    // every zero → all zeros up to g_3000 lie on the critical line.
    // Gram 定律首次失效於 n=126；Rosser 區塊仍計得全部零點。
    auto gr = zc.VerifyCriticalLine(-1, 3000, 8);
    CHECK(gr.verified);
    CHECK(gr.zerosFound == gr.zerosExpected);
    CHECK(!gr.firstFailures.empty() && gr.firstFailures[0] == 126);
    CHECK(gr.unresolvedBlocks.empty());

    // Full audit battery passes at default and extended settings.
    // 審計電池在預設與擴充設定下皆通過。
    auto rd = zc.RunAudit(IQGPU::ZetaCompute::AuditConfig{});
    CHECK(rd.total > 0 && rd.passed == rd.total);
    IQGPU::ZetaCompute::AuditConfig cfg;
    cfg.tMax = 100.0;
    cfg.spectrumTMax = 1000.0;
    cfg.xMax = 100000;
    cfg.gramLast = 2000;
    auto rx = zc.RunAudit(cfg);
    CHECK(rx.total > 0 && rx.passed == rx.total);
    if (rx.passed != rx.total) std::printf("%s\n", rx.ToString().c_str());
    rt.Shutdown();
}

// --- Turing/Brent certification + golden self-test | Turing 認證 + 黃金自檢 ---
void TestTuringCertification() {
    IQGPU::IQGPURuntime rt;
    rt.Initialize();
    IQGPU::ZetaCompute zc(rt);

    auto gr = zc.VerifyCriticalLine(-1, 5000, 8);
    CHECK(gr.verified);
    CHECK(gr.uncertainSigns == 0);
    CHECK(gr.minMarginRatio > 1.0);
    CHECK(gr.turingCertified);
    CHECK(gr.certifiedZeros >= 4990);
    CHECK(gr.certifiedZeros == static_cast<uint64_t>(gr.certifiedGram + 1));
    CHECK(gr.turingBlocksUsed >= 1);

    // Without starting at g_{-1} the lower bound N(g_n) ≥ n+1 is unknown.
    // 不從 g_{-1} 起算時，下界 N(g_n) ≥ n+1 未知 → 不得認證。
    CHECK(!zc.VerifyCriticalLine(100, 2000, 8).turingCertified);

    auto st = zc.SelfTest();
    CHECK(st.total >= 5 && st.passed == st.total);
    if (st.passed != st.total) std::printf("%s\n", st.ToString().c_str());
    rt.Shutdown();
}

// --- SpectralScatter: chunk-seamless + repulsion | 跨 chunk 無接縫 + 排斥 ---
void TestSpectralScatter() {
    IQGPU::IQGPURuntime rt;
    rt.Initialize();
    IQGPU::ZetaCompute zc(rt);
    const auto spec = zc.FindZeros(10.0, 1500.0, 0.02);
    rt.Shutdown();

    Math::Zeta::SpectralScatter::Config cfg;
    cfg.spacing = 4.0f;
    cfg.seed = 99;
    Math::Zeta::SpectralScatter sc(spec, cfg);
    CHECK(sc.IsValid());
    CHECK(sc.MaxRigidityDeviation() < 3.0);   // spectral rigidity | 頻譜剛性

    using S = Math::Zeta::SpectralScatter::Sample;
    auto key = [](const S& a, const S& b) {
        return a.row != b.row ? a.row < b.row : a.index < b.index;
    };
    auto whole = sc.Query(-60.0f, -60.0f, 140.0f, 140.0f);
    std::vector<S> tiled;
    for (int cy = 0; cy < 4; ++cy)
        for (int cx = 0; cx < 4; ++cx) {
            auto part = sc.Query(-60.0f + 50.0f * cx, -60.0f + 50.0f * cy,
                                 -10.0f + 50.0f * cx, -10.0f + 50.0f * cy);
            tiled.insert(tiled.end(), part.begin(), part.end());
        }
    std::sort(whole.begin(), whole.end(), key);
    std::sort(tiled.begin(), tiled.end(), key);
    CHECK(whole.size() == tiled.size());
    bool same = whole.size() == tiled.size();
    for (size_t i = 0; same && i < whole.size(); ++i)
        same = whole[i].row == tiled[i].row && whole[i].index == tiled[i].index &&
               whole[i].x == tiled[i].x && whole[i].y == tiled[i].y;
    CHECK(same);

    // Density ≈ 1 / (s · s√3/2).
    const double expect = 200.0 * 200.0 / (4.0 * 4.0 * 0.8660254);
    CHECK(std::fabs(whole.size() / expect - 1.0) < 0.1);

    // Clumping: Poisson would put ~28% of points within 0.3·s of a neighbour.
    // 群聚：Poisson 約 28% 的點在 0.3·s 內有鄰點。
    size_t close = 0;
    for (size_t i = 0; i < whole.size(); ++i)
        for (size_t j = 0; j < whole.size(); ++j) {
            if (i == j) continue;
            const double dx = whole[i].x - whole[j].x, dy = whole[i].y - whole[j].y;
            if (dx * dx + dy * dy < 1.2 * 1.2) { ++close; break; }
        }
    CHECK(static_cast<double>(close) / whole.size() < 0.10);
}

// --- World::ResourceScatter | 世界資源散佈 ---
void TestResourceScatter() {
    IQGPU::IQGPURuntime rt;
    rt.Initialize();
    IQGPU::ZetaCompute zc(rt);
    const auto spec = zc.FindZeros(10.0, 1500.0, 0.02);
    rt.Shutdown();

    auto build = [&](World::ResourceScatter& rs) {
        rs.AddLayer({1, 6.0f, 1.0f, 0.02f, -10.0f});   // trees, everywhere
        rs.AddLayer({2, 20.0f, 0.7f, 0.01f, 0.3f});    // ore, masked
    };
    World::ResourceScatter a(spec, 2024), b(spec, 2024), c(spec, 7);
    build(a);
    build(b);
    build(c);

    auto ids = [](std::vector<World::ResourceNode> v) {
        std::vector<uint64_t> out;
        for (auto& n : v) out.push_back(n.id);
        std::sort(out.begin(), out.end());
        return out;
    };
    const auto big = a.Generate(0.0f, 0.0f, 256.0f, 256.0f);
    std::vector<World::ResourceNode> chunks;
    for (int cy = 0; cy < 4; ++cy)
        for (int cx = 0; cx < 4; ++cx) {
            auto part = a.GenerateChunk(cx, cy, 64.0f);
            chunks.insert(chunks.end(), part.begin(), part.end());
        }
    const auto idBig = ids(big);
    CHECK(!idBig.empty());
    CHECK(idBig == ids(chunks));                                   // seamless
    CHECK(std::adjacent_find(idBig.begin(), idBig.end()) == idBig.end()); // unique
    CHECK(idBig == ids(b.Generate(0.0f, 0.0f, 256.0f, 256.0f)));   // deterministic
    CHECK(idBig != ids(c.Generate(0.0f, 0.0f, 256.0f, 256.0f)));   // seed matters

    size_t trees = 0, ore = 0;
    for (auto& n : big) (n.type == 1 ? trees : ore)++;
    CHECK(trees > 1000 && ore > 0);
    CHECK(ore < 0.7 * 256.0 * 256.0 / (20.0 * 20.0 * 0.866));      // mask culls
}

// --- Double-double + high-precision Z | 雙倍精度 + 高精度 Z ---
void TestHighPrecisionZ() {
    using namespace Math;
    const DD::dd l2 = DD::Sub(DD::LogD(2.0), DD::kLn2);
    CHECK(std::fabs(DD::ToDouble(l2)) < 1e-30);
    const DD::dd e = DD::Sub(DD::Exp(DD::LogD(10.0)), DD::dd{10.0, 0.0});
    // Measured 4.5e-28 abs (4.5e-29 rel): 10 squarings amplify ~1024×.
    // 實測絕對誤差 4.5e-28（相對 4.5e-29）：10 次平方放大約 1024 倍。
    CHECK(std::fabs(DD::ToDouble(e)) < 1e-26);
    // 12345678901 full turns reduce to ~0 | 整數圈數約化為 ~0
    CHECK(std::fabs(DD::Reduce2Pi(DD::MulD(DD::kTwoPi, 12345678901.0))) < 1e-15);

    // θ_dd agrees with the double asymptotic series at moderate t.
    // 中等 t 時 θ_dd 與 double 漸近級數一致。
    CHECK(std::fabs(DD::ToDouble(Zeta::ThetaDD(1000.0)) - Zeta::RSTheta(1000.0)) < 1e-11);

    // dd Gram point satisfies θ(g_n) = nπ far better than the double one.
    // dd Gram 點滿足 θ(g_n)=nπ，遠優於 double 版本。
    const int64_t n = 1000000000;
    const double g = Zeta::GramPointDD(n);
    DD::dd target = DD::TwoProd(static_cast<double>(n), DD::kPi.hi);
    target = DD::AddD(target, static_cast<double>(n) * DD::kPi.lo);
    const double resid = DD::ToDouble(DD::Sub(Zeta::ThetaDD(g), target));
    const double ulp = std::nextafter(g, 2.0 * g) - g;
    CHECK(std::fabs(resid) <= Zeta::RSThetaPrime(g) * ulp);

    // dd-phase Z matches double HardyZ where the latter is still accurate.
    // 在 double HardyZ 仍精確之處，dd 相位 Z 與其一致。
    Zeta::HighPrecisionZ hpz(2.0e5);
    double worst = 0.0;
    for (double t = 1.0e4; t < 2.0e5; t += 997.3)
        worst = std::max(worst, std::fabs(hpz.Z(t) - Zeta::HardyZ(t)));
    CHECK(worst < 1e-6);
    CHECK(Zeta::HighPrecisionZ::ErrorBound(3.3e9) < 1e-8);
    CHECK(Zeta::HardyZErrorBound(3.3e9) > 1e-3);  // why dd is needed | 需要 dd 的原因
}

// --- Two-sided Turing window at height | 高處雙側 Turing 視窗 ---
void TestTuringWindow() {
    IQGPU::IQGPURuntime rt;
    rt.Initialize();
    IQGPU::ZetaCompute zc(rt);
    auto w = zc.VerifyWindow(100000000, 100001500, 8);
    CHECK(w.highPrecision);
    CHECK(w.verified);
    CHECK(w.uncertainSigns == 0);
    CHECK(w.turingCertified);
    CHECK(w.certifiedGramLo > 100000000 && w.certifiedGram < 100001500);
    CHECK(w.certifiedZeros == static_cast<uint64_t>(w.certifiedGram - w.certifiedGramLo));
    CHECK(w.certifiedZeros > 1400);
    rt.Shutdown();
}

// --- Time-sliced checkpointed job | 時間切片檢查點工作 ---
void TestVerificationJob() {
    IQGPU::IQGPURuntime rt;
    rt.Initialize();
    IQGPU::ZetaCompute zc(rt);
    IQGPU::ZetaVerificationJob job(zc, 8, 3000);
    for (int i = 0; i < 100 && job.GetState().certifiedGram < 30000; ++i) {
        const auto s = job.Step(20.0);
        CHECK(s.segments >= 1);
        if (job.Failed()) break;
    }
    CHECK(!job.Failed());
    const auto st = job.GetState();
    CHECK(st.certifiedGram >= 30000);
    CHECK(st.CertifiedZeros() == static_cast<uint64_t>(st.certifiedGram + 1));
    CHECK(std::fabs(st.certifiedHeight - Math::Zeta::GramPoint(st.certifiedGram)) <
          1e-9 * st.certifiedHeight);

    const std::string path =
        (std::filesystem::temp_directory_path() / "sp_rhjob_test.txt").string();
    CHECK(job.Save(path));
    IQGPU::ZetaVerificationJob resumed(zc, 8);
    CHECK(resumed.Load(path));
    CHECK(resumed.GetState().certifiedGram == st.certifiedGram);
    resumed.Step(20.0);
    CHECK(!resumed.Failed());
    CHECK(resumed.GetState().certifiedGram > st.certifiedGram);

    // Corrupt one digit → checksum rejects it | 改壞一位數 → 校驗拒絕
    {
        std::ifstream in(path, std::ios::binary);
        std::string text((std::istreambuf_iterator<char>(in)), {});
        in.close();
        const size_t at = text.find("certified_gram=") + 15;
        text[at] = text[at] == '9' ? '8' : static_cast<char>(text[at] + 1);
        std::ofstream(path, std::ios::binary | std::ios::trunc) << text;
    }
    IQGPU::ZetaVerificationJob bad(zc, 8);
    CHECK(!bad.Load(path));
    std::filesystem::remove(path);
    rt.Shutdown();
}

int main() {
    std::printf("=== StonePotatoes Math + IQGPU Tests ===\n");

    TestRiemannZeta();
    TestZeroSpectrum();
    TestSpectralSampler();
    TestSpectralNoise();
    TestCPUBackend();
    TestComputeGraph();
    TestQuantumInspired();
    TestZetaCompute();
    TestRHVerification();
    TestTuringCertification();
    TestSpectralScatter();
    TestResourceScatter();
    TestHighPrecisionZ();
    TestTuringWindow();
    TestVerificationJob();

    std::printf("=== %d passed, %d failed ===\n", g_Passed, g_Failed);
    return g_Failed == 0 ? 0 : 1;
}
