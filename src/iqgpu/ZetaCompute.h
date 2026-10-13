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
