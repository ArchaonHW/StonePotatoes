#pragma once

#include "ZetaCompute.h"
#include <cstdint>
#include <string>

// ZetaVerificationJob — time-sliced, resumable RH verification.
// Engine pattern for long-running background compute: each Step(budgetMs)
// runs whole certified segments until the frame/idle budget is used, adapts
// the segment size to the measured cost, and chains segments through the
// Turing anchor (N(g_n) = n+1 certified ⇒ next segment starts at g_n).
// Progress persists to a checksummed checkpoint so it survives restarts.
// ZetaVerificationJob — 時間切片、可續跑的 RH 驗證。
// 長時間背景計算的引擎模式：每次 Step(budgetMs) 在幀/閒置預算內執行
// 完整的認證分段，依實測成本自適應分段大小，並透過 Turing 錨點串接
// （已認證 N(g_n)=n+1 ⇒ 下一段由 g_n 開始）。進度寫入含校驗碼的檢查點，
// 重啟後可續跑。
namespace StonePotatoes {
namespace IQGPU {

class ZetaVerificationJob {
public:
    struct State {
        int64_t certifiedGram = -1;     // N(g_n) = n+1 certified up to n
        double certifiedHeight = 0.0;   // g_n
        uint64_t segments = 0;
        uint64_t samplesEvaluated = 0;
        double computeMs = 0.0;
        std::string failure;            // non-empty → job halted | 非空即停止
        uint64_t CertifiedZeros() const {
            return static_cast<uint64_t>(certifiedGram + 1);
        }
    };

    struct StepResult {
        uint32_t segments = 0;
        int64_t gramAdvanced = 0;
        double ms = 0.0;
    };

    ZetaVerificationJob(ZetaCompute& zeta, uint32_t samplesPerInterval = 8,
                        int64_t initialSegment = 20000);

    // Runs ≥ 1 segment (if not failed) and keeps going while within budget.
    // 執行 ≥1 個分段（未失敗時），並在預算內持續執行。
    StepResult Step(double budgetMs);

    bool Failed() const { return !m_State.failure.empty(); }
    const State& GetState() const { return m_State; }
    int64_t SegmentSize() const { return m_Segment; }

    // Checkpoint: "key=value" text + FNV-1a checksum (corruption detection,
    // not tamper-proofing). Load also re-derives g_n to sanity-check.
    // 檢查點：key=value 文字 + FNV-1a 校驗（偵測損毀，非防竄改）。
    // 載入時重算 g_n 做一致性檢查。
    bool Save(const std::string& path) const;
    bool Load(const std::string& path);

private:
    ZetaCompute& m_Zeta;
    uint32_t m_Samples;
    int64_t m_Segment;
    State m_State;
};

} // namespace IQGPU
} // namespace StonePotatoes
