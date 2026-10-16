#include "rendering/SpriteAnimation2D.h"

#include <array>
#include <cstdio>

using StonePotatoes::Rendering::SpriteAnimation2D;
using StonePotatoes::Rendering::SpriteAnimationFrame2D;

namespace {
int failures = 0;

void Check(bool condition, const char* name) {
    if (!condition) {
        std::fprintf(stderr, "[FAIL] %s\n", name);
        ++failures;
    }
}

} // namespace

int main() {
    const std::array<SpriteAnimationFrame2D, 3> frames{{
        {{0.0f, 0.0f, 0.25f, 1.0f}, 2},
        {{0.25f, 0.0f, 0.5f, 1.0f}, 1},
        {{0.5f, 0.0f, 0.75f, 1.0f}, 3},
    }};
    SpriteAnimation2D animation;
    Check(animation.Configure(frames), "configure valid atlas frames");
    Check(animation.FrameIndex() == 0, "starts at first frame");
    animation.Advance(1);
    Check(animation.FrameIndex() == 0, "duration holds current frame");
    animation.Advance(1);
    Check(animation.FrameIndex() == 1, "advances on exact duration");
    animation.Advance(4);
    Check(animation.FrameIndex() == 0, "large tick advance wraps loop");
    animation.Advance(6);
    Check(animation.FrameIndex() == 0, "whole cycle leaves phase unchanged");

    Check(animation.Configure(frames, SpriteAnimation2D::Playback::Once),
          "configure one-shot playback");
    animation.Advance(100);
    Check(animation.IsFinished(), "one-shot reports completion");
    Check(animation.FrameIndex() == 2, "one-shot holds final frame");
    animation.Reset();
    Check(!animation.IsFinished() && animation.FrameIndex() == 0,
          "reset restarts playback");

    const std::array<SpriteAnimationFrame2D, 1> zeroDuration{{
        {{0.0f, 0.0f, 1.0f, 1.0f}, 0},
    }};
    Check(!animation.Configure(zeroDuration), "rejects zero duration");
    Check(animation.IsValid(), "invalid configuration preserves prior clip");

    return failures == 0 ? 0 : 1;
}
