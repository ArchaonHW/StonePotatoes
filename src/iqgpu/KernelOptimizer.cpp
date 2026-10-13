#include "KernelOptimizer.h"
#include <algorithm>
#include <vector>
#include <limits>

namespace StonePotatoes {
namespace IQGPU {

KernelOptimizer::Result
KernelOptimizer::Optimize(const std::vector<Variant>& variants,
                          GPU::CPUBackend& backend,
                          uint32_t repeats, uint32_t warmupRuns) {
    Result r;
    r.repeats = repeats;
    r.bestMs = std::numeric_limits<double>::max();
    r.worstMs = 0.0;

    for (const auto& v : variants) {
        VariantResult vr;
        vr.name = v.name;

        std::vector<double> samples;
        samples.reserve(repeats);

        // Warmup — not recorded | 暖身 — 不計入
        for (uint32_t w = 0; w < warmupRuns; ++w) {
            if (v.useGrainStealing) {
                backend.ParallelFor(v.threadCount,
                    [&v](uint32_t gid) { v.kernel.Invoke(gid, v.userData); },
                    v.grainSize);
            } else {
                GPU::DispatchParams p;
                p.threadCount = v.threadCount;
                p.userData = v.userData;
                backend.Dispatch(v.kernel, p);
            }
        }

        for (uint32_t rep = 0; rep < repeats; ++rep) {
            GPU::CPUBackend::Stats s;
            if (v.useGrainStealing) {
                s = backend.ParallelFor(v.threadCount,
                    [&v](uint32_t gid) { v.kernel.Invoke(gid, v.userData); },
                    v.grainSize);
            } else {
                GPU::DispatchParams p;
                p.threadCount = v.threadCount;
                p.userData = v.userData;
                s = backend.Dispatch(v.kernel, p);
            }
            samples.push_back(s.dispatchMs);
            vr.workersUsed = s.threadsUsed;
            vr.itemsProcessed = s.itemsProcessed;
        }

        std::sort(samples.begin(), samples.end());
        vr.bestMs = samples.front();
        vr.worstMs = samples.back();
        vr.medianMs = samples[samples.size() / 2];

        if (vr.medianMs < r.bestMs) {
            r.bestMs = vr.medianMs;
            r.bestName = vr.name;
        }
        if (vr.medianMs > r.worstMs) {
            r.worstMs = vr.medianMs;
            r.worstName = vr.name;
        }
        r.variants.push_back(vr);
    }

    if (r.bestMs > 0.0) r.speedup = r.worstMs / r.bestMs;
    return r;
}

} // namespace IQGPU
} // namespace StonePotatoes
