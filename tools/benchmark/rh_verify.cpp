// StonePotatoesRHVerify — large-scale RH finite verification on IQGPU.
// StonePotatoesRHVerify — 在 IQGPU 上做大規模 RH 有限驗證。
//
// Usage | 用法:
//   StonePotatoesRHVerify [gramLast=100000] [samples=8] [--audit] [--selftest]
//       origin mode: certify N(g_n) = n+1 from g_{-1} | 自原點認證
//   StonePotatoesRHVerify --window <nFirst> <count> [samples=8]
//       two-sided Turing window at any height | 任意高度雙側 Turing 視窗
//   StonePotatoesRHVerify --job <checkpoint> <budgetMs> [samples=8]
//       time-sliced resumable job; loads/saves checkpoint | 時間切片可續跑
//
// Output is key=value lines (CI/agent friendly). Exit code 0 only when the
// requested checks all pass. FINITE VERIFICATION — never a proof.
// 輸出為 key=value 行（便於 CI/agent 解析）。全部通過才回傳 0。
// 有限驗證——永遠不是證明。

#include "iqgpu/IQGPURuntime.h"
#include "iqgpu/ZetaCompute.h"
#include "iqgpu/ZetaVerificationJob.h"
#include "math/RiemannZeta.h"
#include "math/HighPrecisionZ.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

using namespace StonePotatoes;

namespace {

void PrintReport(const IQGPU::ZetaCompute::GramReport& r, uint32_t workers) {
    std::printf("gram.range=[%lld,%lld] t=[%.4f,%.4f]\n",
                static_cast<long long>(r.firstGram), static_cast<long long>(r.lastGram),
                r.tLo, r.tHi);
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
    std::printf("signs.samples=%llu uncertain=%llu uncertain_gram=%llu min_margin=%.2fx "
                "bound_at_top=%.3g\n",
                static_cast<unsigned long long>(r.samplesEvaluated),
                static_cast<unsigned long long>(r.uncertainSigns),
                static_cast<unsigned long long>(r.uncertainGramPoints), r.minMarginRatio,
                r.highPrecision ? Math::Zeta::HighPrecisionZ::ErrorBound(r.tHi)
                                : Math::Zeta::HardyZErrorBound(r.tHi));
    std::printf("turing.certified=%d K=%lld zeros_in(g_%lld=%.6f, g_%lld=%.6f]=%llu\n",
                r.turingCertified ? 1 : 0, static_cast<long long>(r.turingBlocksUsed),
                static_cast<long long>(r.certifiedGramLo), r.certifiedHeightLo,
                static_cast<long long>(r.certifiedGram), r.certifiedHeight,
                static_cast<unsigned long long>(r.certifiedZeros));
    std::printf("perf.ms=%.1f workers=%u gpu=%d samples_per_sec=%.3g\n", r.ms,
                workers, r.gpuAccelerated ? 1 : 0,
                r.ms > 0 ? 1000.0 * static_cast<double>(r.samplesEvaluated) / r.ms : 0.0);
}

} // namespace

int main(int argc, char** argv) {
    std::string mode = "origin", checkpoint;
    int64_t gramLast = 100000, windowFirst = 0, windowCount = 0;
    double budgetMs = 0.0;
    uint32_t samples = 8;
    bool audit = false, selfTest = false;
    int pos = 0;
    for (int i = 1; i < argc; ++i) {
        if (!std::strcmp(argv[i], "--audit")) audit = true;
        else if (!std::strcmp(argv[i], "--selftest")) selfTest = true;
        else if (!std::strcmp(argv[i], "--window") && i + 2 < argc) {
            mode = "window";
            windowFirst = std::atoll(argv[++i]);
            windowCount = std::atoll(argv[++i]);
        } else if (!std::strcmp(argv[i], "--job") && i + 2 < argc) {
            mode = "job";
            checkpoint = argv[++i];
            budgetMs = std::atof(argv[++i]);
        } else if (mode == "origin" && pos == 0) { gramLast = std::atoll(argv[i]); ++pos; }
        else { samples = static_cast<uint32_t>(std::atoi(argv[i])); }
    }
    if (samples < 2 ||
        (mode == "origin" && gramLast < 10) ||
        (mode == "window" && (windowFirst < 0 || windowCount < 100)) ||
        (mode == "job" && budgetMs <= 0.0)) {
        std::fprintf(stderr, "bad arguments (origin gramLast >= 10; window count >= 100; "
                             "job budgetMs > 0; samples >= 2)\n");
        return 2;
    }

    IQGPU::IQGPURuntime rt;
    rt.Initialize();
    IQGPU::ZetaCompute zc(rt);
    const uint32_t workers = rt.GetBackend().WorkerCount();
    bool ok = true;

    if (selfTest) {
        const auto st = zc.SelfTest();
        std::printf("%s\n", st.ToString().c_str());
        ok = ok && st.passed == st.total;
    }

    if (mode == "origin") {
        const auto r = zc.VerifyCriticalLine(-1, gramLast, samples);
        PrintReport(r, workers);
        ok = ok && r.verified && r.turingCertified;
    } else if (mode == "window") {
        const auto r = zc.VerifyWindow(windowFirst, windowFirst + windowCount, samples);
        PrintReport(r, workers);
        ok = ok && r.verified && r.turingCertified;
    } else {
        IQGPU::ZetaVerificationJob job(zc, samples);
        const bool resumed = job.Load(checkpoint);
        const auto before = job.GetState().certifiedGram;
        const auto step = job.Step(budgetMs);
        const auto& s = job.GetState();
        std::printf("job.resumed=%d from_gram=%lld segments=%u advanced=%lld ms=%.1f\n",
                    resumed ? 1 : 0, static_cast<long long>(before), step.segments,
                    static_cast<long long>(step.gramAdvanced), step.ms);
        std::printf("job.certified N(g_%lld=%.6f)=%llu total_segments=%llu total_ms=%.1f "
                    "next_segment=%lld\n",
                    static_cast<long long>(s.certifiedGram), s.certifiedHeight,
                    static_cast<unsigned long long>(s.CertifiedZeros()),
                    static_cast<unsigned long long>(s.segments), s.computeMs,
                    static_cast<long long>(job.SegmentSize()));
        if (job.Failed()) std::printf("job.failure=%s\n", s.failure.c_str());
        const bool saved = job.Save(checkpoint);
        std::printf("job.checkpoint=%s saved=%d\n", checkpoint.c_str(), saved ? 1 : 0);
        ok = ok && !job.Failed() && saved;
    }

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
