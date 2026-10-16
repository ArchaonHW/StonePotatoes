#pragma once

#include <glm/glm.hpp>

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace StonePotatoes::Rendering {

struct SpriteAnimationFrame2D {
    glm::vec4 uvRect{0.0f, 0.0f, 1.0f, 1.0f};
    std::uint32_t durationTicks = 1;
};

// Presentation-only atlas animation advanced by explicit integer ticks.
// The caller owns the clock and passes the current UV to DrawSprite2D.
class SpriteAnimation2D final {
public:
    enum class Playback { Loop, Once };

    bool Configure(std::span<const SpriteAnimationFrame2D> frames,
                   Playback playback = Playback::Loop);
    void Reset();
    void Advance(std::uint64_t ticks = 1);

    bool IsValid() const { return !m_Frames.empty(); }
    bool IsFinished() const { return m_Finished; }
    std::size_t FrameIndex() const { return m_FrameIndex; }
    const glm::vec4& UVRect() const;

private:
    std::vector<SpriteAnimationFrame2D> m_Frames;
    Playback m_Playback = Playback::Loop;
    std::size_t m_FrameIndex = 0;
    std::uint32_t m_ElapsedTicks = 0;
    std::uint64_t m_CycleTicks = 0;
    bool m_Finished = false;
};

} // namespace StonePotatoes::Rendering
