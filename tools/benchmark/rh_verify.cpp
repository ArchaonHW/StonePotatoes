// StonePotatoesRHVerify — large-scale RH finite verification on IQGPU.
// StonePotatoesRHVerify — 在 IQGPU 上做大規模 RH 有限驗證。
//
// Usage | 用法:
//   StonePotatoesRHVerify [gramLast=100000] [samples=8] [--audit] [--selftest]
//
// Output is key=value lines (CI/agent friendly). Exit code 0 only when the
// requested checks all pass. FINITE VERIFICATION — never a proof.
// 輸出為 key=value 行（便於 CI/agent 解析）。全部通過才回傳 0。
// 有限驗證——永遠不是證明。

#include "iqgpu/IQGPURuntime.h"
#include "iqgpu/ZetaCompute.h"
#include "math/RiemannZeta.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

using namespace StonePotatoes;

int main(int argc, char** argv) {
    int64_t gramLast = 100000;
    uint32_t samples = 8;
    bool audit = false, selfTest = false;
    int pos = 0;
    for (int i = 1; i < argc; ++i) {
        if (!std::strcmp(argv[i], "--audit")) audit = true;
        else if (!std::strcmp(argv[i], "--selftest")) selfTest = true;
        else if (pos == 0) { gramLast = std::atoll(argv[i]); ++pos; }
        else if (pos == 1) { samples = static_cast<uint32_t>(std::atoi(argv[i])); ++pos; }
    }
    if (gramLast < 10 || gramLast > 10000000 || samples < 2) {
        std::fprintf(stderr, "gramLast in [10, 1e7], samples >= 2\n");
        return 2;
    }

    IQGPU::IQGPURuntime rt;
    rt.Initialize();
    IQGPU::ZetaCompute zc(rt);
    bool ok = true;

    if (selfTest) {
        const auto st = zc.SelfTest();
        std::printf("%s\n", st.ToString().c_str());
        ok = ok && st.passed == st.total;
    }

    const auto r = zc.VerifyCriticalLine(-1, gramLast, samples);
    std::printf("gram.range=[-1,%lld] t=[%.4f,%.4f]\n",
                static_cast<long long>(gramLast), r.tLo, r.tHi);
    std::printf("zeros.found=%llu expected=%llu\n",
                static_cast<unsigned long long>(r.zerosFound),
                static_cast<unsigned long long>(r.zerosExpected));
    std::printf("gram.law_failures=%llu rosser_blocks=%llu refined_intervals=%llu "
                "unresolved_blocks=%zu\n",
                static_cast<unsigned long long>(r.gramLawFailures),
                static_cast<unsigned long long>(r.rosserBlocks),
                static_cast<unsigned long long>(r.refinedIntervals),
                r.unresolvedBlocks.size());
    for (const int64_t b : r.unresolvedBlocks)
        std::printf("unresolved.block_start=%lld\n", static_cast<long long>(b));
    std::printf("signs.samples=%llu uncertain=%llu min_margin=%.2fx\n",
                static_cast<unsigned long long>(r.samplesEvaluated),
                static_cast<unsigned long long>(r.uncertainSigns), r.minMarginRatio);
    std::printf("turing.certified=%d K=%lld N(%.6f)=%llu\n", r.turingCertified ? 1 : 0,
                static_cast<long long>(r.turingBlocksUsed), r.certifiedHeight,
                static_cast<unsigned long long>(r.certifiedZeros));
    std::printf("perf.ms=%.1f workers=%u samples_per_sec=%.3g\n", r.ms,
                rt.GetBackend().WorkerCount(),
                r.ms > 0 ? 1000.0 * static_cast<double>(r.samplesEvaluated) / r.ms : 0.0);
    ok = ok && r.verified && r.turingCertified;

    if (audit) {
        IQGPU::ZetaCompute::AuditConfig cfg;
        cfg.tMax = 100.0;
        cfg.spectrumTMax = 1000.0;
        cfg.xMax = 100000;
        cfg.gramLast = 2000;
        const auto a = zc.RunAudit(cfg);
        std::printf("%s\n", a.ToString().c_str());
        ok = ok && a.passed == a.total;
    }

    std::printf("result=%s finite_verification=1 proof=0\n", ok ? "PASS" : "FAIL");
    rt.Shutdown();
    return ok ? 0 : 1;
}
