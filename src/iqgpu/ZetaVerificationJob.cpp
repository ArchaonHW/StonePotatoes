#include "ZetaVerificationJob.h"
#include "../math/RiemannZeta.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <map>
#include <sstream>

namespace StonePotatoes {
namespace IQGPU {

namespace {
uint64_t Fnv1a(const std::string& s) {
    uint64_t h = 1469598103934665603ull;
    for (const unsigned char c : s) {
        h ^= c;
        h *= 1099511628211ull;
    }
    return h;
}
constexpr const char* kMagic = "stonepotatoes.rhjob v1";
} // namespace

ZetaVerificationJob::ZetaVerificationJob(ZetaCompute& zeta,
                                         uint32_t samplesPerInterval,
                                         int64_t initialSegment)
    : m_Zeta(zeta),
      m_Samples(std::max<uint32_t>(2, samplesPerInterval)),
      m_Segment(std::max<int64_t>(1000, initialSegment)) {}

ZetaVerificationJob::StepResult ZetaVerificationJob::Step(double budgetMs) {
    StepResult res;
    if (Failed() || budgetMs <= 0.0) return res;
    const auto t0 = std::chrono::steady_clock::now();
    auto elapsed = [&] {
        return std::chrono::duration<double, std::milli>(
                   std::chrono::steady_clock::now() - t0)
            .count();
    };
    do {
        const int64_t from = m_State.certifiedGram;
        auto r = m_Zeta.VerifyCriticalLine(from, from + m_Segment, m_Samples, true);
        if (!r.turingCertified) {
            // One retry at 4× sampling before declaring an anomaly.
            // 宣告異常前以 4 倍取樣重試一次。
            r = m_Zeta.VerifyCriticalLine(from, from + m_Segment, m_Samples * 4, true);
        }
        m_State.samplesEvaluated += r.samplesEvaluated;
        m_State.computeMs += r.ms;
        if (!r.turingCertified || r.certifiedGram <= from) {
            char buf[256];
            std::snprintf(buf, sizeof(buf),
                          "segment from g_%lld uncertified: verified=%d "
                          "unresolved_blocks=%zu uncertain_signs=%llu",
                          static_cast<long long>(from), r.verified ? 1 : 0,
                          r.unresolvedBlocks.size(),
                          static_cast<unsigned long long>(r.uncertainSigns));
            m_State.failure = buf;
            break;
        }
        res.gramAdvanced += r.certifiedGram - from;
        m_State.certifiedGram = r.certifiedGram;
        m_State.certifiedHeight = r.certifiedHeight;
        ++m_State.segments;
        ++res.segments;
        // Aim each segment at ~half the budget (cost grows ~√t with height).
        // 每段目標約半個預算（成本隨高度約 √t 成長）。
        const double msPerGram = r.ms / static_cast<double>(m_Segment);
        if (msPerGram > 0.0)
            m_Segment = std::clamp<int64_t>(
                static_cast<int64_t>(0.5 * budgetMs / msPerGram), 1000, 2000000);
    } while (elapsed() < budgetMs);
    res.ms = elapsed();
    return res;
}

bool ZetaVerificationJob::Save(const std::string& path) const {
    char buf[512];
    std::snprintf(buf, sizeof(buf),
                  "%s\ncertified_gram=%lld\ncertified_height=%.17g\nsegments=%llu\n"
                  "samples=%llu\ncompute_ms=%.3f\nsegment_size=%lld\n",
                  kMagic, static_cast<long long>(m_State.certifiedGram),
                  m_State.certifiedHeight,
                  static_cast<unsigned long long>(m_State.segments),
                  static_cast<unsigned long long>(m_State.samplesEvaluated),
                  m_State.computeMs, static_cast<long long>(m_Segment));
    const std::string body = buf;
    std::ofstream f(path, std::ios::binary | std::ios::trunc);
    if (!f) return false;
    f << body << "checksum=" << Fnv1a(body) << "\n";
    return static_cast<bool>(f);
}

bool ZetaVerificationJob::Load(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return false;
    std::string line, body;
    std::map<std::string, std::string> kv;
    uint64_t checksum = 0;
    bool first = true, haveSum = false;
    while (std::getline(f, line)) {
        if (first) {
            if (line != kMagic) return false;
            body += line + "\n";
            first = false;
            continue;
        }
        const size_t eq = line.find('=');
        if (eq == std::string::npos) return false;
        const std::string k = line.substr(0, eq), v = line.substr(eq + 1);
        if (k == "checksum") {
            checksum = std::stoull(v);
            haveSum = true;
            break;
        }
        kv[k] = v;
        body += line + "\n";
    }
    if (!haveSum || checksum != Fnv1a(body)) return false;
    try {
        State s;
        s.certifiedGram = std::stoll(kv.at("certified_gram"));
        s.certifiedHeight = std::stod(kv.at("certified_height"));
        s.segments = std::stoull(kv.at("segments"));
        s.samplesEvaluated = std::stoull(kv.at("samples"));
        s.computeMs = std::stod(kv.at("compute_ms"));
        const int64_t seg = std::stoll(kv.at("segment_size"));
        // The anchor must really be Gram point g_n.
        // 錨點必須確實是 Gram 點 g_n。
        if (s.certifiedGram >= 0) {
            const double g = Math::Zeta::GramPoint(s.certifiedGram);
            if (std::fabs(g - s.certifiedHeight) > 1e-9 * g) return false;
        }
        m_State = s;
        m_Segment = std::max<int64_t>(1000, seg);
    } catch (...) {
        return false;
    }
    return true;
}

} // namespace IQGPU
} // namespace StonePotatoes
