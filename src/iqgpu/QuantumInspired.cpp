#include "QuantumInspired.h"
#include <cmath>
#include <algorithm>
#include <limits>
#include <numeric>
#include <unordered_set>

namespace StonePotatoes {
namespace IQGPU {

// ---------------------------------------------------------------------------
// SimulatedAnnealer
// ---------------------------------------------------------------------------

SimulatedAnnealer::Result
SimulatedAnnealer::Optimize(const std::vector<double>& initial,
                            EnergyFn energy, NeighborFn neighbor,
                            uint32_t iterations, double t0,
                            double coolingRate, uint32_t seed) {
    Result r;
    std::mt19937 rng(seed);
    std::uniform_real_distribution<double> u(0.0, 1.0);

    std::vector<double> cur = initial;
    double curE = energy(cur);
    r.initialEnergy = curE;
    r.bestState = cur;
    r.bestEnergy = curE;

    double T = t0;
    for (uint32_t i = 0; i < iterations; ++i) {
        std::vector<double> cand = cur;
        neighbor(cand, rng);
        const double candE = energy(cand);
        const double dE = candE - curE;
        if (dE < 0 || u(rng) < std::exp(-dE / T)) {
            cur = std::move(cand);
            curE = candE;
            if (curE < r.bestEnergy) {
                r.bestEnergy = curE;
                r.bestState = cur;
            }
        }
        T *= coolingRate;
    }
    r.iterations = iterations;
    r.improvement = r.initialEnergy - r.bestEnergy;
    return r;
}

// ---------------------------------------------------------------------------
// QuantumInspiredAnnealer
// ---------------------------------------------------------------------------

QuantumInspiredAnnealer::Result
QuantumInspiredAnnealer::Optimize(
    const std::vector<double>& initial, EnergyFn energy,
    NeighborFn neighbor,
    std::function<void(std::vector<double>&, std::mt19937&)> teleport,
    uint32_t iterations, double t0, double coolingRate,
    double tunnelingProb, uint32_t seed) {
    Result r;
    std::mt19937 rng(seed);
    std::uniform_real_distribution<double> u(0.0, 1.0);

    std::vector<double> cur = initial;
    double curE = energy(cur);
    r.initialEnergy = curE;
    r.bestState = cur;
    r.bestEnergy = curE;

    double T = t0;
    for (uint32_t i = 0; i < iterations; ++i) {
        std::vector<double> cand = cur;
        // Transverse-field analog: with prob p, perform a long-range
        // "tunnel" jump rather than a local move.
        // 橫向場類比：以機率 p 執行長程「穿隧」跳躍而非局部移動。
        if (u(rng) < tunnelingProb) {
            teleport(cand, rng);
        } else {
            neighbor(cand, rng);
        }
        const double candE = energy(cand);
        const double dE = candE - curE;
        // Tunneling jumps are always evaluated (no Boltzmann barrier);
        // local moves use the annealing acceptance rule.
        // 穿隧跳躍一律評估（無玻茲曼障壁）；
        // 局部移動使用退火接受規則。
        if (dE < 0 || u(rng) < std::exp(-dE / T)) {
            cur = std::move(cand);
            curE = candE;
            if (curE < r.bestEnergy) {
                r.bestEnergy = curE;
                r.bestState = cur;
            }
        }
        T *= coolingRate;
    }
    r.iterations = iterations;
    r.improvement = r.initialEnergy - r.bestEnergy;
    return r;
}

// ---------------------------------------------------------------------------
// AmplitudeAmplifier — real amplitude-vector Grover iteration
// ---------------------------------------------------------------------------

AmplitudeAmplifier::Result
AmplitudeAmplifier::Search(uint32_t domainSize, Oracle oracle,
                           uint32_t maxRounds, uint32_t seed) {
    Result r;
    r.domainSize = domainSize;
    if (domainSize == 0) return r;

    std::mt19937 rng(seed);
    (void)rng;

    // Identify marked states (classical evaluation of the oracle —
    // the honest cost is O(N); the quantum speedup is NOT claimed).
    // 識別標記狀態（對神諭做傳統評估 —
    // 誠實的複雜度為 O(N)；不宣稱量子加速）。
    std::unordered_set<uint32_t> marked;
    for (uint32_t i = 0; i < domainSize; ++i) {
        if (oracle(i)) marked.insert(i);
    }
    r.markedStates.assign(marked.begin(), marked.end());
    std::sort(r.markedStates.begin(), r.markedStates.end());

    const uint32_t M = static_cast<uint32_t>(marked.size());
    if (M == 0) {
        r.probabilities.assign(domainSize, 0.0);
        return r;
    }

    // Uniform superposition | 均勻疊加
    const double amp0 = 1.0 / std::sqrt(static_cast<double>(domainSize));
    std::vector<double> amp(domainSize, amp0);

    // Optimal Grover rounds: floor(pi/4 * sqrt(N/M))
    // 最佳 Grover 輪數：floor(pi/4 * sqrt(N/M))
    const double ratio =
        static_cast<double>(domainSize) / static_cast<double>(M);
    uint32_t optimal = static_cast<uint32_t>(
        std::floor((3.14159265358979323846 / 4.0) * std::sqrt(ratio)));
    const uint32_t rounds = std::min(std::max(optimal, 1u), maxRounds);

    for (uint32_t round = 0; round < rounds; ++round) {
        // Oracle: phase-flip marked amplitudes | 神諭：翻轉標記振幅相位
        for (uint32_t m : marked) amp[m] = -amp[m];

        // Inversion about the mean | 相對於平均值的反轉
        double mean = 0.0;
        for (double a : amp) mean += a;
        mean /= domainSize;
        for (double& a : amp) a = 2.0 * mean - a;
    }
    r.roundsApplied = rounds;

    // Probabilities = |amp|^2 | 機率 = |振幅|^2
    r.probabilities.resize(domainSize);
    for (uint32_t i = 0; i < domainSize; ++i)
        r.probabilities[i] = amp[i] * amp[i];
    return r;
}

// ---------------------------------------------------------------------------
// ProbabilisticStateSpace
// ---------------------------------------------------------------------------

ProbabilisticStateSpace::ProbabilisticStateSpace(uint32_t numStates,
                                               uint32_t seed)
    : m_NumStates(numStates), m_Amplitudes(numStates, 0.0), m_Rng(seed) {}

void ProbabilisticStateSpace::SetAmplitude(uint32_t state, double amp) {
    if (state < m_NumStates) m_Amplitudes[state] = amp;
}

double ProbabilisticStateSpace::GetAmplitude(uint32_t state) const {
    return state < m_NumStates ? m_Amplitudes[state] : 0.0;
}

double ProbabilisticStateSpace::GetProbability(uint32_t state) const {
    if (state >= m_NumStates) return 0.0;
    double total = 0.0;
    for (double a : m_Amplitudes) total += a * a;
    if (total <= 0.0) return 0.0;
    const double a = m_Amplitudes[state];
    return (a * a) / total;
}

void ProbabilisticStateSpace::Normalize() {
    double total = 0.0;
    for (double a : m_Amplitudes) total += a * a;
    const double norm = std::sqrt(total);
    if (norm <= 0.0) return;
    for (double& a : m_Amplitudes) a /= norm;
}

void ProbabilisticStateSpace::Interfere(
    const ProbabilisticStateSpace& other, double a, double b) {
    const size_t n = std::min(m_Amplitudes.size(), other.m_Amplitudes.size());
    for (size_t i = 0; i < n; ++i) {
        m_Amplitudes[i] = a * m_Amplitudes[i] + b * other.m_Amplitudes[i];
    }
}

uint32_t ProbabilisticStateSpace::Measure() {
    double total = 0.0;
    for (double a : m_Amplitudes) total += a * a;
    if (total <= 0.0) return 0;
    std::uniform_real_distribution<double> u(0.0, total);
    const double pick = u(m_Rng);
    double acc = 0.0;
    for (uint32_t i = 0; i < m_NumStates; ++i) {
        acc += m_Amplitudes[i] * m_Amplitudes[i];
        if (pick <= acc) return i;
    }
    return m_NumStates - 1;
}

} // namespace IQGPU
} // namespace StonePotatoes
