#pragma once

#include <cstdint>
#include <vector>

// ZetaSpectral — engine-facing uses of the ζ zero spectrum.
//   ZeroSpectrum         : zero table + GUE / Montgomery statistics
//                          (the "random-matrix fingerprint" checks from the
//                          PotatoEngine RH explorer, as reusable API)
//   SpectralPointSampler : level-repulsion point placement — unfolded zero
//                          spacings follow GUE statistics, so points never
//                          clump (unlike Poisson) yet never look gridded.
//                          Use for trees / rocks / settlements / NPC spawns.
//   SpectralNoise        : deterministic aperiodic noise whose frequencies
//                          are zero ordinates γ_k (conjecturally linearly
//                          independent over Q → the pattern never repeats).
// ZetaSpectral — ζ 零點頻譜的引擎端應用。
//   ZeroSpectrum         ：零點表 + GUE / Montgomery 統計
//                          （PotatoEngine RH 探索器的「隨機矩陣指紋」檢查，API 化）
//   SpectralPointSampler ：層級排斥點分佈——展開後零點間距服從 GUE 統計，
//                          點不會群聚（不同於 Poisson），也不會呈網格感。
//                          用於樹木/岩石/聚落/NPC 生成點。
//   SpectralNoise        ：以零點縱座標 γ_k 為頻率的確定性非週期雜訊
//                          （猜想 γ_k 在 Q 上線性獨立 → 圖樣永不重複）。
namespace StonePotatoes {
namespace Math {
namespace Zeta {

// GUE Wigner surmise p(s) = (32/π²)s²e^{−4s²/π} and its CDF.
// GUE Wigner surmise 密度與累積分佈。
double WignerSurmiseGUE(double s);
double WignerSurmiseGUECdf(double s);

// Montgomery pair correlation R₂(u) = 1 − (sin πu / πu)².
// Montgomery 配對關聯 R₂(u)。
double MontgomeryR2(double u);

class ZeroSpectrum {
public:
    struct Histogram {
        double binWidth = 0.0;
        std::vector<double> measured;   // density per bin | 每 bin 量測密度
        std::vector<double> reference;  // model density per bin | 理論密度
        uint64_t samples = 0;
        double MaxAbsError() const;
    };

    ZeroSpectrum() = default;
    explicit ZeroSpectrum(std::vector<double> gammas);

    // Sequential sign-change scan of HardyZ on [tLo, tHi]. For large ranges
    // prefer IQGPU::ZetaCompute::FindZeros (parallel).
    // 循序掃描 HardyZ 變號；大範圍請用 IQGPU::ZetaCompute::FindZeros（平行）。
    static ZeroSpectrum Compute(double tLo, double tHi, double step = 0.02);

    const std::vector<double>& Gammas() const { return m_Gammas; }
    size_t Size() const { return m_Gammas.size(); }
    bool Empty() const { return m_Gammas.empty(); }

    // Unfolded ordinates x_i = θ(γ_i)/π + 1 (mean spacing 1).
    // 展開座標 x_i = θ(γ_i)/π+1（平均間距 1）。
    std::vector<double> Unfolded() const;

    // Nearest-neighbour spacings on the unfolded scale.
    // 展開刻度上的最近鄰間距。
    std::vector<double> NormalizedSpacings() const;

    Histogram SpacingHistogram(double sMax = 3.0, int bins = 6) const;
    Histogram PairCorrelation(double uMax = 3.0, int bins = 6) const;

    // Kolmogorov–Smirnov distance of spacings vs GUE / Poisson.
    // 間距相對 GUE / Poisson 的 KS 距離。
    double KSDistanceGUE() const;
    double KSDistancePoisson() const;

    // Smallest normalized spacing (Lehmer-pair detector).
    // 最小正規化間距（Lehmer 近重合偵測）。
    double MinNormalizedSpacing(double* atGamma = nullptr) const;

private:
    std::vector<double> m_Gammas;
};

class SpectralPointSampler {
public:
    struct Point2 {
        float x = 0.0f;
        float y = 0.0f;
    };

    SpectralPointSampler() = default;
    explicit SpectralPointSampler(const ZeroSpectrum& spectrum);

    bool IsValid() const { return m_Spacings.size() >= 8; }
    size_t SpacingCount() const { return m_Spacings.size(); }

    // Points in [0, length) with mean gap meanSpacing; deterministic per seed.
    // [0,length) 內平均間距 meanSpacing 的點；同 seed 結果確定。
    std::vector<float> Sample1D(float length, float meanSpacing,
                                uint32_t seed) const;

    // Rows at meanSpacing·√3/2 (hex-like packing); each row is an
    // independent spectral 1D sequence with its own offset.
    // 列距 meanSpacing·√3/2（近六角堆積）；每列為獨立偏移的頻譜一維序列。
    std::vector<Point2> Sample2D(float width, float height, float meanSpacing,
                                 uint32_t seed) const;

private:
    std::vector<double> m_Spacings;  // mean exactly 1 | 平均恰為 1
};

// SpectralScatter — chunk-seamless 2D scatter for streaming worlds.
// Uses spectral rigidity: unfolded zeros satisfy x_k − k = S(γ_k) with S(T)
// small and bounded, so point k of a row sits within maxDev spacings of k·s.
// Any point is therefore addressable by (row, index) without walking the
// sequence → querying one big rect or any tiling of it gives the SAME set,
// with GUE level repulsion intact across chunk borders.
// SpectralScatter — 串流世界用、跨 chunk 無接縫的 2D 散佈。
// 利用頻譜剛性：展開零點滿足 x_k − k = S(γ_k)，S(T) 小且有界，
// 故列上第 k 點必落在 k·s 的 maxDev 個間距內。任一點可由（列、索引）
// 直接定址，無需逐點走訪 → 查詢一個大矩形或其任意切塊，結果「完全相同」，
// 且 GUE 層級排斥跨越 chunk 邊界仍成立。
class SpectralScatter {
public:
    struct Config {
        float spacing = 4.0f;    // mean distance along a row | 列上平均間距
        uint32_t seed = 1;
        float rowJitter = 0.25f; // ≤ 0.5 of row height | 列高比例抖動（≤0.5）
    };
    struct Sample {
        float x = 0.0f;
        float y = 0.0f;
        int64_t row = 0;         // stable identity (row, index) | 穩定身分
        int64_t index = 0;
    };

    SpectralScatter() = default;
    SpectralScatter(const ZeroSpectrum& spectrum, const Config& cfg);

    bool IsValid() const { return m_U.size() >= 8; }
    const Config& GetConfig() const { return m_Cfg; }

    // Max |x_k − k| over the table, in spacings (≈ max|S(T)|).
    // 表內 max|x_k − k|（以間距計，≈ max|S(T)|）。
    double MaxRigidityDeviation() const { return m_MaxDev; }

    // All samples with x ∈ [x0,x1), y ∈ [y0,y1) (half-open → exact tiling).
    // 半開區間 [x0,x1)×[y0,y1) 內所有樣本（半開 → 切塊精確分割）。
    std::vector<Sample> Query(float x0, float y0, float x1, float y1) const;

private:
    double U(int64_t k) const;   // global unfolded position | 全域展開位置
    std::vector<double> m_U;     // u[j] − u[0], mean spacing 1 | 平均間距 1
    double m_MaxDev = 0.0;
    Config m_Cfg;
};

class SpectralNoise {
public:
    SpectralNoise() = default;
    // terms: number of zeros used (≤ spectrum size). Output is zero-mean,
    // unit-variance; frequency of term k = baseFrequency·γ_k/γ_0.
    // terms：使用零點數（≤ 頻譜大小）。輸出零均值、單位變異；
    // 第 k 項頻率 = baseFrequency·γ_k/γ_0。
    SpectralNoise(const ZeroSpectrum& spectrum, uint32_t seed, int terms = 32,
                  double baseFrequency = 1.0);

    bool IsValid() const { return !m_Freq.empty(); }
    double Evaluate(double x) const;
    double Evaluate(double x, double y) const;

private:
    std::vector<double> m_Freq, m_Amp, m_Phase, m_DirX, m_DirY;
};

} // namespace Zeta
} // namespace Math
} // namespace StonePotatoes
