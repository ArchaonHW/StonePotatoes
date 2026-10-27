// SPDX-License-Identifier: MIT
// Copyright (c) 2026 ArchaonHW
#pragma once

#include <cstdint>

namespace StonePotatoes {
namespace Civilization {
namespace Military {

/** Inputs for a deterministic, simultaneous single-volley engagement. */
struct EngagementConfig {
    float attackerUnits = 0.0f;
    float defenderUnits = 0.0f;
    float attackerDamagePerUnit = 0.0f;
    float defenderDamagePerUnit = 0.0f;
    // Damage points required to remove one unit on the respective side.
    float attackerDurability = 1.0f;
    float defenderDurability = 1.0f;
};

/** Aggregate losses and survivors after both sides exchange one volley. */
struct EngagementResult {
    float attackerLosses = 0.0f;
    float defenderLosses = 0.0f;
    float survivingAttackers = 0.0f;
    float survivingDefenders = 0.0f;
};

/** Aggregate losses and survivors after a bounded multi-round battle. */
struct BattleResult {
    float attackerLosses = 0.0f;
    float defenderLosses = 0.0f;
    float survivingAttackers = 0.0f;
    float survivingDefenders = 0.0f;
    std::uint32_t roundsCompleted = 0;
};

inline constexpr std::uint32_t MaxBattleRounds = 10000;

/** Resolve one simultaneous volley; losses are bounded by each side's units. */
EngagementResult ResolveEngagement(const EngagementConfig& config);

/**
 * Repeat simultaneous volleys until a side is eliminated or maxRounds is met.
 * Throws if maxRounds exceeds MaxBattleRounds.
 */
BattleResult ResolveBattle(const EngagementConfig& config,
                           std::uint32_t maxRounds);

} // namespace Military
} // namespace Civilization
} // namespace StonePotatoes
