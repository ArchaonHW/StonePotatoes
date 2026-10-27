// SPDX-License-Identifier: MIT
// Copyright (c) 2026 ArchaonHW
#include "civilization/military/BattleResolution.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace StonePotatoes {
namespace Civilization {
namespace Military {

EngagementResult ResolveEngagement(const EngagementConfig& config) {
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

} // namespace Military
} // namespace Civilization
} // namespace StonePotatoes
