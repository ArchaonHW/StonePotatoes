#pragma once

#include "../../math/ZetaSpectral.h"
#include <cstdint>
#include <vector>

// ResourceScatter — streaming, chunk-seamless resource / prop placement for
// world generation, built on the ζ zero spectrum:
//   • SpectralScatter  → positions: GUE level repulsion (no clumps, no grid),
//                        identical regardless of chunk size / load order
//   • SpectralNoise    → biome mask: aperiodic, so large maps never tile
//   • stable node ids  → (layer, row, index) hash — safe for save files,
//                        lockstep networking and "harvested" bitsets
// ResourceScatter — 以 ζ 零點頻譜為基礎、可串流且跨 chunk 無接縫的
// 資源/道具放置：
//   • SpectralScatter → 位置：GUE 層級排斥（不群聚、無網格感），
//                        與 chunk 大小/載入順序無關
//   • SpectralNoise   → 生態遮罩：非週期，大地圖永不重複
//   • 穩定節點 id     → (layer,row,index) 雜湊——可用於存檔、鎖步網路與
//                        「已採集」位元集
namespace StonePotatoes {
namespace World {

struct ResourceLayer {
    uint32_t type = 0;          // game-defined resource type | 遊戲自訂資源類型
    float spacing = 8.0f;       // mean node distance (world units) | 平均間距
    float density = 1.0f;       // keep probability ∈ [0,1] | 保留機率
    float maskScale = 0.01f;    // biome-mask frequency | 生態遮罩頻率
    float maskThreshold = -10;  // keep where mask > threshold (−10 = always)
                                // 遮罩 > 門檻才保留（−10 = 恆保留）
};

struct ResourceNode {
    float x = 0.0f;
    float y = 0.0f;
    uint32_t type = 0;
    uint64_t id = 0;            // stable across sessions | 跨工作階段穩定
};

class ResourceScatter {
public:
    ResourceScatter(Math::Zeta::ZeroSpectrum spectrum, uint32_t worldSeed);

    void AddLayer(const ResourceLayer& layer);
    size_t LayerCount() const { return m_Layers.size(); }

    // Nodes in world rect [x0,x1)×[y0,y1) (half-open → exact chunk tiling).
    // 世界矩形 [x0,x1)×[y0,y1) 內的節點（半開 → chunk 精確分割）。
    std::vector<ResourceNode> Generate(float x0, float y0, float x1, float y1) const;

    // Square chunk (cx, cy) of side chunkSize.
    // 邊長 chunkSize 的方形 chunk (cx, cy)。
    std::vector<ResourceNode> GenerateChunk(int32_t cx, int32_t cy,
                                            float chunkSize) const;

private:
    struct Layer {
        ResourceLayer cfg;
        Math::Zeta::SpectralScatter scatter;
        Math::Zeta::SpectralNoise mask;
        uint64_t salt = 0;
    };
    Math::Zeta::ZeroSpectrum m_Spectrum;
    uint32_t m_Seed;
    std::vector<Layer> m_Layers;
};

} // namespace World
} // namespace StonePotatoes
