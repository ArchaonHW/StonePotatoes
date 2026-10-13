#pragma once

#include <cstdint>
#include <functional>
#include <random>
#include <vector>

// Quantum-inspired algorithms on classical hardware.
// IMPORTANT: this is classical simulation of quantum-inspired
// techniques — NOT universal quantum computing and NOT quantum
// hardware. All complexity remains classical.
// 在傳統硬體上執行的量子啟發演算法。
// 重要：這是量子啟發技術的傳統模擬 —
// 並非通用量子計算，也非量子硬體。所有複雜度仍屬傳統計算。
namespace StonePotatoes {
namespace IQGPU {

// ---------------------------------------------------------------------------
// SimulatedAnnealer — classical baseline
// SimulatedAnnealer — 傳統基準演算法
// ---------------------------------------------------------------------------
class SimulatedAnnealer {
public:
    struct Result {
        std::vector<double> bestState;
        double bestEnergy = 0.0;
        uint32_t iterations = 0;
        double initialEnergy = 0.0;
        double improvement = 0.0;   // initial - best
    };

    using EnergyFn = std::function<double(const std::vector<double>&)>;
    using NeighborFn = std::function<void(std::vector<double>&, std::mt19937&)>;

    Result Optimize(const std::vector<double>& initial,
                    EnergyFn energy, NeighborFn neighbor,
                    uint32_t iterations, double t0 = 1.0,
                    double coolingRate = 0.995, uint32_t seed = 42);
};

// ---------------------------------------------------------------------------
// QuantumAnnealer — quantum-INSPIRED: adds tunneling jumps to escape
// local minima, plus a transverse-field-like fluctuation term.
// QuantumAnnealer — 量子「啟發」：加入穿隧跳躍以逃離局部最小值，
// 並加入類橫向場的擾動項。
// ---------------------------------------------------------------------------
class QuantumInspiredAnnealer {
public:
    using Result = SimulatedAnnealer::Result;
    using EnergyFn = SimulatedAnnealer::EnergyFn;
    using NeighborFn = SimulatedAnnealer::NeighborFn;

    // tunnelingProb: probability per iteration of a long-range jump
    // instead of a local neighbor step (simulated tunneling).
    // tunnelingProb：每次迭代以長程跳躍取代局部鄰近步的機率
    // （模擬穿隧）。
    Result Optimize(const std::vector<double>& initial,
                    EnergyFn energy, NeighborFn neighbor,
                    std::function<void(std::vector<double>&, std::mt19937&)> teleport,
                    uint32_t iterations, double t0, double coolingRate,
                    double tunnelingProb = 0.05, uint32_t seed = 42);
};

// ---------------------------------------------------------------------------
// AmplitudeAmplifier — Grover-inspired search over a discrete domain.
// Maintains a real amplitude vector, applies oracle phase-flip +
// inversion-about-mean each round. Converges to marked states.
// AmplitudeAmplifier — Grover 啟發的離散域搜尋。
// 維護真實振幅向量，每輪施加神諭相位翻轉 +
// 相對於平均值的反轉。收斂至標記狀態。
// ---------------------------------------------------------------------------
class AmplitudeAmplifier {
public:
    // oracle returns true for marked states | 神諭對標記狀態回傳 true
    using Oracle = std::function<bool(uint32_t)>;

    struct Result {
        std::vector<uint32_t> markedStates;
        std::vector<double> probabilities;   // post-amplification
        uint32_t roundsApplied = 0;
        uint32_t domainSize = 0;
    };

    // Runs ceil(pi/4 * sqrt(N/M)) rounds, or maxRounds, whichever is
    // smaller — mirrors the real Grover bound.
    // 執行 ceil(pi/4 * sqrt(N/M)) 輪或 maxRounds（取較小者）—
    // 對齊真實 Grover 上界。
    Result Search(uint32_t domainSize, Oracle oracle,
                  uint32_t maxRounds = 32, uint32_t seed = 1234);
};

// ---------------------------------------------------------------------------
// ProbabilisticStateSpace — superposition-inspired state evaluation.
// States carry amplitude weights; measurement samples proportionally
// to amplitude^2 (Born rule, classically). Interference is modeled as
// additive amplitude combination.
// ProbabilisticStateSpace — 疊加啟發的狀態評估。
// 狀態帶有振幅權重；依振幅平方（玻恩定則，傳統方式）
// 比例取樣測量。干涉以振幅加法組合建模。
// ---------------------------------------------------------------------------
class ProbabilisticStateSpace {
public:
    explicit ProbabilisticStateSpace(uint32_t numStates,
                                     uint32_t seed = 7);

    void SetAmplitude(uint32_t state, double amplitude);
    double GetAmplitude(uint32_t state) const;
    double GetProbability(uint32_t state) const;   // |amp|^2 / sum|amp|^2

    // Normalize so sum(|amp|^2) == 1 | 正規化使 sum(|amp|^2) == 1
    void Normalize();

    // Additive interference: amp' = a*amp1 + b*amp2 (component-wise)
    // 加法干涉：amp' = a*amp1 + b*amp2（逐分量）
    void Interfere(const ProbabilisticStateSpace& other,
                   double a, double b);

    // Born-rule measurement | 玻恩定則測量
    uint32_t Measure();

    uint32_t StateCount() const { return m_NumStates; }

private:
    uint32_t m_NumStates;
    std::vector<double> m_Amplitudes;
    std::mt19937 m_Rng;
};

} // namespace IQGPU
} // namespace StonePotatoes
