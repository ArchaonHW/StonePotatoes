// SPDX-License-Identifier: MIT
// Copyright (c) 2026 ArchaonHW
#include "civilization/military/BattleResolution.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace StonePotatoes {
namespace Civilization {
namespace Military {
namespace {

void ValidateEngagement(const EngagementConfig& config) {
    if (!std::isfinite(config.attackerUnits) || config.attackerUnits < 0.0f ||
        !std::isfinite(config.defenderUnits) || config.defenderUnits < 0.0f ||
        !std::isfinite(config.attackerDamagePerUnit) ||
        config.attackerDamagePerUnit < 0.0f ||
        !std::isfinite(config.defenderDamagePerUnit) ||
        config.defenderDamagePerUnit < 0.0f ||
        !std::isfinite(config.attackerDurability) ||
        config.attackerDurability <= 0.0f ||
        !std::isfinite(config.defenderDurability) ||
        config.defenderDurability <= 0.0f) {
        throw std::invalid_argument("invalid engagement configuration");
    }
}

EngagementResult ResolveEngagementUnchecked(const EngagementConfig& config) {
    const double attackerLosses = std::min(
        static_cast<double>(config.attackerUnits),
        static_cast<double>(config.defenderUnits) *
            config.defenderDamagePerUnit / config.attackerDurability);
    const double defenderLosses = std::min(
        static_cast<double>(config.defenderUnits),
        static_cast<double>(config.attackerUnits) *
            config.attackerDamagePerUnit / config.defenderDurability);

    return {
        static_cast<float>(attackerLosses),
        static_cast<float>(defenderLosses),
        static_cast<float>(config.attackerUnits - attackerLosses),
        static_cast<float>(config.defenderUnits - defenderLosses),
    };
}

} // namespace

EngagementResult ResolveEngagement(const EngagementConfig& config) {
    ValidateEngagement(config);
    return ResolveEngagementUnchecked(config);
}

BattleResult ResolveBattle(const EngagementConfig& config,
                           std::uint32_t maxRounds) {
    ValidateEngagement(config);
    if (maxRounds > MaxBattleRounds) {
        throw std::invalid_argument("battle round limit exceeds supported maximum");
    }

    float remainingAttackers = config.attackerUnits;
    float remainingDefenders = config.defenderUnits;
    std::uint32_t roundsCompleted = 0;
    while (roundsCompleted < maxRounds && remainingAttackers > 0.0f &&
           remainingDefenders > 0.0f) {
        EngagementConfig volleyConfig = config;
        volleyConfig.attackerUnits = remainingAttackers;
        volleyConfig.defenderUnits = remainingDefenders;
        const EngagementResult volley =
            ResolveEngagementUnchecked(volleyConfig);
        remainingAttackers = volley.survivingAttackers;
        remainingDefenders = volley.survivingDefenders;
        ++roundsCompleted;
        if (volley.attackerLosses == 0.0f && volley.defenderLosses == 0.0f) {
            break;
        }
    }

    return {config.attackerUnits - remainingAttackers,
            config.defenderUnits - remainingDefenders,
            remainingAttackers,
            remainingDefenders,
            roundsCompleted};
}

} // namespace Military
} // namespace Civilization
} // namespace StonePotatoes
