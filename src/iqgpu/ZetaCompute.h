#pragma once

#include "../gpu/CPUBackend.h"
#include "../math/ZetaSpectral.h"
#include <cstdint>
#include <string>
#include <vector>

namespace StonePotatoes {
namespace IQGPU {

class IQGPURuntime;

// IQGPU::ZetaCompute — parallel ζ zero scanning + critical-line audit
// on the IQGPU backend. The O(N·√t) sample scan is the expensive part;
// the CPU backend only wall-clock-accelerates it (honest complexity:
// still classical work — no quantum speedup is claimed).
// IQGPU::ZetaCompute — 在 IQGPU 後端上平行掃描 ζ 零點並稽核臨界線。
// O(N·√t) 取樣掃描為耗時部分；CPU 後端僅縮短時鐘時間
// （誠實複雜度：仍是傳統工作量 — 未宣稱量子加速）。
class ZetaCompute {
public:
    explicit ZetaCompute(IQGPURuntime& runtime);

    // Parallel Hardy-Z evaluation over a list of t values.
    // 對 t 值列表平行求值 Hardy-Z。
    std::vector<double> EvaluateZ(const std::vector<double>& ts,
                                  GPU::CPUBackend::Stats* stats = nullptr);

    // Parallel sign-change + bisection scan → zero spectrum.
    // 平行變號掃描 + 二分求精 → 零點頻譜。
    Math::Zeta::ZeroSpectrum FindZeros(
        double tLo, double tHi, double step,
        GPU::CPUBackend::Stats* stats = nullptr);

    // Gram/Rosser-block critical-line verification report.
    // Gram / Rosser 區塊臨界線驗證報告。
    struct GramReport {
        int64_t firstGram = 0;
        int64_t lastGram = 0;
        double tLo = 0.0;
        double tHi = 0.0;
        uint64_t gramPoints = 0;
        uint64_t rosserBlocks = 0;
        uint64_t zerosExpected = 0;
        uint64_t zerosFound = 0;
        uint64_t gramLawFailures = 0;
        uint64_t refinedIntervals = 0;
        std::vector<int64_t> firstFailures;
        std::vector<int64_t> unresolvedBlocks;
        bool verified = false;
        double ms = 0.0;

        // Sign certainty: every counted sample satisfied |Z| > error bound
        // (Gabcke); samples inside the band are nudged, survivors counted.
        // 符號確定性：每個計數樣本皆滿足 |Z| > 誤差界（Gabcke）；
        // 落在誤差帶內的樣本會被微移重取，仍不確定者計入此數。
        uint64_t samplesEvaluated = 0;
        uint64_t uncertainSigns = 0;
        double minMarginRatio = 0.0;    // min |Z|/bound over samples | 最小餘裕倍數

        // Turing/Brent certification (requires nFirst = −1): N(g_n) = n+1
        // exactly, i.e. every zero with 0 < γ < g_n is on the critical line.
        // Turing/Brent 認證（需 nFirst=−1）：N(g_n)=n+1 精確成立，
        // 即 0<γ<g_n 的所有零點皆在臨界線上。
        bool turingCertified = false;
        int64_t certifiedGram = 0;      // n | 認證 Gram 索引
        double certifiedHeight = 0.0;   // g_n | 認證高度
        uint64_t certifiedZeros = 0;    // N(g_n) = n + 1
        int64_t turingBlocksUsed = 0;   // K (Brent bound) | Brent 區塊數
    };
    GramReport VerifyCriticalLine(int64_t nFirst, int64_t nLast,
                                  uint32_t samplesPerInterval = 64);

    // Self-audit: on-line zero count, argument-principle strip count,
    // Gram/Rosser verification, GUE fingerprint, explicit formula,
    // Mertens/Robin bounds, Li λ₁, de Bruijn–Newman zeros.
    // 自我稽核：線上零點計數、輻角原理條帶計數、Gram/Rosser 驗證、
    // GUE 指紋、顯式公式、Mertens/Robin 界、Li λ₁、dBN 零點。
    struct AuditConfig {
        double tMax = 60.0;           // winding contour top | 輻角矩形頂邊
        double spectrumTMax = 200.0;  // zero-scan range | 零點掃描範圍
        uint32_t xMax = 20000;        // arithmetic bound | 算術界
        int64_t gramLast = 40;        // Gram range end | Gram 範圍末端
        double debruijnLambda = 0.2;  // dBN flow parameter | dBN 流參數
    };
    struct AuditReport {
        struct Item {
            std::string name;
            std::string value;
            bool pass = false;
        };
        std::vector<Item> items;
        int total = 0;
        int passed = 0;
        double ms = 0.0;
        std::string ToString() const;
    };
    AuditReport RunAudit(const AuditConfig& cfg);

    // Golden-value backend self-test — a known-answer compute workload for
    // validating any IQGPU backend (today CPU; future GPU/QPU): N(1000)=649,
    // Odlyzko zeros, Gram failure at n=126, and bit-identical Z(t) between
    // a 1-worker and an N-worker dispatch (determinism for lockstep/replays).
    // 黃金值後端自檢——用於驗證任何 IQGPU 後端（現為 CPU，未來 GPU/QPU）
    // 的已知答案計算負載：N(1000)=649、Odlyzko 零點、Gram 於 n=126 失效、
    // 以及 1-worker 與 N-worker 派發的 Z(t) 位元一致（鎖步/重播確定性）。
    AuditReport SelfTest();

    // Free-style scan helpers (backend passed directly, no runtime).
    // 自由式掃描（直接傳後端，不需 runtime）。
    struct ScanResult {
        std::vector<double> gammas;  // refined zero ordinates | 求精後零點縱座標
        uint32_t samplesEvaluated = 0;
        uint32_t signChanges = 0;
        double scanMs = 0.0;    // parallel sampling time | 平行取樣時間
        double refineMs = 0.0;  // bisection time | 二分求精時間
    };

    // Parallel zero scan over [tLo, tHi); step must resolve zero spacing.
    // [tLo,tHi) 平行零點掃描；step 須解析零點間距。
    static ScanResult FindZeros(GPU::CPUBackend& backend,
                                double tLo, double tHi, double step = 0.02);

    // Sequential reference — same math, one thread; honest speedup baseline.
    // 循序參考實作 — 相同數學、單執行緒；誠實加速比基準。
    static ScanResult FindZerosSequential(double tLo, double tHi,
                                          double step = 0.02);

private:
    IQGPURuntime& m_Runtime;
};

} // namespace IQGPU
} // namespace StonePotatoes
