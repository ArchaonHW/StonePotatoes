#include "ResourceScatter.h"

namespace StonePotatoes {
namespace World {

namespace {
uint64_t Mix(uint64_t x) {
    x += 0x9E3779B97F4A7C15ull;
    x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ull;
    x = (x ^ (x >> 27)) * 0x94D049BB133111EBull;
    return x ^ (x >> 31);
}
} // namespace

ResourceScatter::ResourceScatter(Math::Zeta::ZeroSpectrum spectrum,
                                 uint32_t worldSeed)
    : m_Spectrum(std::move(spectrum)), m_Seed(worldSeed) {}

void ResourceScatter::AddLayer(const ResourceLayer& layer) {
    Layer l;
    l.cfg = layer;
    l.salt = Mix((static_cast<uint64_t>(m_Seed) << 32) ^ layer.type ^
                 (static_cast<uint64_t>(m_Layers.size()) << 48));
    Math::Zeta::SpectralScatter::Config sc;
    sc.spacing = layer.spacing;
    sc.seed = static_cast<uint32_t>(l.salt);
    l.scatter = Math::Zeta::SpectralScatter(m_Spectrum, sc);
    l.mask = Math::Zeta::SpectralNoise(m_Spectrum, static_cast<uint32_t>(l.salt >> 32),
                                       24, 1.0);
    m_Layers.push_back(std::move(l));
}

std::vector<ResourceNode> ResourceScatter::Generate(float x0, float y0, float x1,
                                                    float y1) const {
    std::vector<ResourceNode> out;
    for (const Layer& l : m_Layers) {
        for (const auto& s : l.scatter.Query(x0, y0, x1, y1)) {
            const uint64_t id =
                Mix(l.salt ^ Mix(static_cast<uint64_t>(s.row) * 0xD6E8FEB86659FD93ull ^
                                 static_cast<uint64_t>(s.index)));
            // Density gate from the id itself → deterministic per node.
            // 由 id 本身決定密度篩選 → 每節點確定。
            const double u = static_cast<double>(id >> 11) * (1.0 / 9007199254740992.0);
            if (u >= l.cfg.density) continue;
            if (l.cfg.maskThreshold > -10.0f &&
                l.mask.Evaluate(s.x * l.cfg.maskScale, s.y * l.cfg.maskScale) <=
                    l.cfg.maskThreshold)
                continue;
            out.push_back({s.x, s.y, l.cfg.type, id});
        }
    }
    return out;
}

std::vector<ResourceNode> ResourceScatter::GenerateChunk(int32_t cx, int32_t cy,
                                                         float chunkSize) const {
    const float x0 = static_cast<float>(cx) * chunkSize;
    const float y0 = static_cast<float>(cy) * chunkSize;
    return Generate(x0, y0, static_cast<float>(cx + 1) * chunkSize,
                    static_cast<float>(cy + 1) * chunkSize);
}

} // namespace World
} // namespace StonePotatoes
