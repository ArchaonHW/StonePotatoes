#pragma once

#include "../gpu/CPUBackend.h"
#include "../gpu/VulkanBackend.h"
#include "../math/ZetaSpectral.h"
#include "../math/DoubleDouble.h"
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace StonePotatoes {
namespace Math { namespace Zeta { class HighPrecisionZ; } }
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
    ~ZetaCompute();  // out-of-line: unique_ptr to incomplete type | 前向型別

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
        uint64_t uncertainSigns = 0;      // after nudging | 微移後仍不確定
        uint64_t uncertainGramPoints = 0; // merged into bulk blocks | 併入區塊
        bool highPrecision = false;       // dd-phase RS path used | 使用 dd 相位路徑
        bool gpuAccelerated = false;      // Vulkan zeta kernel sampled | GPU 取樣
        double minMarginRatio = 0.0;    // min |Z|/bound over samples | 最小餘裕倍數

        // Turing/Brent certification: every zero with g_lo < γ ≤ g_hi is on
        // the critical line and there are exactly certifiedZeros of them.
        // From the origin (lo = −1) this is N(g_hi) = hi + 1.
        // Turing/Brent 認證：g_lo < γ ≤ g_hi 的零點全在臨界線上，且恰有
        // certifiedZeros 個。自原點（lo=−1）起算即 N(g_hi)=hi+1。
        bool turingCertified = false;
        int64_t certifiedGramLo = -1;   // lo | 認證下端 Gram 索引
        double certifiedHeightLo = 0.0; // g_lo
        int64_t certifiedGram = 0;      // hi | 認證上端 Gram 索引
        double certifiedHeight = 0.0;   // g_hi | 認證高度
        uint64_t certifiedZeros = 0;    // hi − lo
        int64_t turingBlocksUsed = 0;   // K (Brent bound) | Brent 區塊數
    };

    // Anchored verification. Certifies when the lower end is exactly known:
    // nFirst = −1 (N(g_{−1}) = 0), or startCertified = true meaning a prior
    // report certified N(g_nFirst) = nFirst + 1 (chain segments this way).
    // 錨定驗證。下端精確已知時才認證：nFirst=−1（N(g_{−1})=0），或
    // startCertified=true 表示先前報告已認證 N(g_nFirst)=nFirst+1（藉此串接分段）。
    GramReport VerifyCriticalLine(int64_t nFirst, int64_t nLast,
                                  uint32_t samplesPerInterval = 64,
                                  bool startCertified = false);

    // Two-sided Turing window at ANY height (e.g. around the 10⁹-th zero):
    // Brent bounds at both ends certify all zeros in (g_lo, g_hi].
    // 任意高度的雙側 Turing 視窗（如第 10⁹ 個零點附近）：
    // 兩端 Brent 界共同認證 (g_lo, g_hi] 內的所有零點。
    GramReport VerifyWindow(int64_t nFirst, int64_t nLast,
                            uint32_t samplesPerInterval = 8);

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

    // --- GPU batch evaluation | GPU 批次求值 ---
    // EvaluateZGPU — one workgroup per t on the Vulkan backend; phases are
    // double-double identical to the CPU HighPrecisionZ path (same dd ops
    // in GLSL). Returns {} when no GPU exists → caller falls back to CPU.
    // θ(t) and the log/sqrt tables come from the CPU (log tables uploaded
    // once and cached by capacity); the correction term is applied on CPU.
    // EvaluateZGPU — Vulkan 後端上每個 t 一個 workgroup；相位為與 CPU
    // HighPrecisionZ 相同的 double-double 運算（GLSL 實作同組 dd 運算）。
    // 無 GPU 時回傳空 → 呼叫端退回 CPU。θ 與對數表在 CPU 準備
    // （表按容量上傳並快取）；修正項在 CPU 套用。
    std::vector<double> EvaluateZGPU(const std::vector<Math::DD::dd>& ts);

    // Disable GPU acceleration for verification paths.
    // 關閉驗證路徑的 GPU 加速。
    void SetUseGPU(bool v) { m_UseGPU = v; }
    bool GetUseGPU() const { return m_UseGPU; }

private:
    enum class CertMode { None, Anchored, Window };
    GramReport VerifyGram(int64_t nFirst, int64_t nLast,
                          uint32_t samplesPerInterval, CertMode mode);

    // GPU tables for EvaluateZGPU — sized to the largest t requested so
    // far; rebuilt if a larger t arrives. 依需求重建的 GPU 對數表。
    bool EnsureGpuTables(double tMax);

    IQGPURuntime& m_Runtime;
    bool m_UseGPU = true;
    std::unique_ptr<Math::Zeta::HighPrecisionZ> m_GpuHp;
    double m_GpuTMax = 0.0;
    GPU::VulkanBackend::BufferHandle m_GpuLogK;
    GPU::VulkanBackend::BufferHandle m_GpuInvSqrtK;
    GPU::VulkanBackend::KernelHandle m_GpuKernel;
};

} // namespace IQGPU
} // namespace StonePotatoes
