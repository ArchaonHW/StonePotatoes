#include "SpriteAnimation2D.h"

#include <cmath>
#include <limits>

namespace StonePotatoes::Rendering {
namespace {

bool IsValidUv(const glm::vec4& uv) {
    return std::isfinite(uv.x) && std::isfinite(uv.y) &&
           std::isfinite(uv.z) && std::isfinite(uv.w) && uv.x >= 0.0f &&
           uv.y >= 0.0f && uv.z <= 1.0f && uv.w <= 1.0f && uv.x < uv.z &&
           uv.y < uv.w;
}

} // namespace

bool SpriteAnimation2D::Configure(
    std::span<const SpriteAnimationFrame2D> frames, Playback playback) {
    if (frames.empty() || frames.size() > 4096) return false;

    std::uint64_t cycleTicks = 0;
    for (const SpriteAnimationFrame2D& frame : frames) {
        if (!IsValidUv(frame.uvRect) || frame.durationTicks == 0 ||
            cycleTicks > std::numeric_limits<std::uint64_t>::max() -
                             frame.durationTicks) {
            return false;
        }
        cycleTicks += frame.durationTicks;
    }

    m_Frames.assign(frames.begin(), frames.end());
    m_Playback = playback;
    m_CycleTicks = cycleTicks;
    Reset();
    return true;
}

void SpriteAnimation2D::Reset() {
    m_FrameIndex = 0;
    m_ElapsedTicks = 0;
    m_Finished = false;
}

void SpriteAnimation2D::Advance(std::uint64_t ticks) {
    if (m_Frames.empty() || m_Finished || ticks == 0) return;

    std::uint64_t remaining =
        m_Frames[m_FrameIndex].durationTicks - m_ElapsedTicks;
    if (ticks < remaining) {
        m_ElapsedTicks += static_cast<std::uint32_t>(ticks);
        return;
    }

    ticks -= remaining;
    if (m_Playback == Playback::Once &&
        m_FrameIndex + 1 == m_Frames.size()) {
        m_ElapsedTicks = m_Frames[m_FrameIndex].durationTicks;
        m_Finished = true;
        return;
    }

    m_FrameIndex = (m_FrameIndex + 1) % m_Frames.size();
    m_ElapsedTicks = 0;
    if (m_Playback == Playback::Loop)
        ticks %= m_CycleTicks;

    while (ticks >= m_Frames[m_FrameIndex].durationTicks) {
        ticks -= m_Frames[m_FrameIndex].durationTicks;
        if (m_Playback == Playback::Once &&
            m_FrameIndex + 1 == m_Frames.size()) {
            m_ElapsedTicks = m_Frames[m_FrameIndex].durationTicks;
            m_Finished = true;
            return;
        }
        m_FrameIndex = (m_FrameIndex + 1) % m_Frames.size();
    }
    m_ElapsedTicks = static_cast<std::uint32_t>(ticks);
}

const glm::vec4& SpriteAnimation2D::UVRect() const {
    static const glm::vec4 fullAtlas{0.0f, 0.0f, 1.0f, 1.0f};
    return m_Frames.empty() ? fullAtlas : m_Frames[m_FrameIndex].uvRect;
}

} // namespace StonePotatoes::Rendering
