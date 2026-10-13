// StonePotatoes math + IQGPU module tests.
// StonePotatoes 數學 + IQGPU 模組測試。

#include "math/RiemannZeta.h"
#include "math/ZetaSpectral.h"
#include "gpu/CPUBackend.h"
#include "gpu/ComputeGraph.h"
#include "gpu/Kernel.h"
#include "iqgpu/QuantumInspired.h"
#include "iqgpu/ZetaCompute.h"
#include "iqgpu/IQGPURuntime.h"

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

    std::printf("=== %d passed, %d failed ===\n", g_Passed, g_Failed);
    return g_Failed == 0 ? 0 : 1;
}
