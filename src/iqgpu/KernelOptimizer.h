#pragma once

#include "../gpu/CPUBackend.h"
#include "../gpu/Kernel.h"
#include <string>
#include <vector>

// KernelOptimizer — bridges the Self-Developing Agent and IQGPU.
// Given several real kernel variants, it benchmarks each on the
// active backend and selects the fastest. This is measurement-driven
// self-optimization, not a heuristic claim.
// KernelOptimizer — 橋接自我開發 Agent 與 IQGPU。
// 對多個真實核心變體在當前後端上基準測試，選出最快者。
// 這是量測驅動的自我優化，而非啟發式宣稱。
namespace StonePotatoes {
namespace IQGPU {

class KernelOptimizer {
public:
    struct Variant {
        std::string name;
        GPU::Kernel kernel;
        uint32_t threadCount = 0;
        uint32_t workerCount = 0;   // 0 = backend default
        void* userData = nullptr;
        bool useGrainStealing = false; // ParallelFor path vs Dispatch
        uint32_t grainSize = 1024;
    };

    struct VariantResult {
        std::string name;
        double medianMs = 0.0;
        double bestMs = 0.0;
        double worstMs = 0.0;
        uint32_t workersUsed = 0;
        uint32_t itemsProcessed = 0;
    };

    struct Result {
        std::vector<VariantResult> variants;
        std::string bestName;
        double bestMs = 0.0;
        std::string worstName;
        double worstMs = 0.0;
        double speedup = 1.0;   // worst / best
        uint32_t repeats = 0;
    };

    // Benchmarks every variant `repeats` times on `backend`, reports
    // median/best/worst per variant and overall winner.
    // 對每個變體在 `backend` 上基準測試 `repeats` 次，
    // 回報各變體的中位/最佳/最差以及整體勝者。
    Result Optimize(const std::vector<Variant>& variants,
                    GPU::CPUBackend& backend,
                    uint32_t repeats = 3,
                    uint32_t warmupRuns = 1);
};

} // namespace IQGPU
} // namespace StonePotatoes
