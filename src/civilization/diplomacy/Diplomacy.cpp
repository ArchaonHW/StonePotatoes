// SPDX-License-Identifier: MIT
// Copyright (c) 2026 ArchaonHW
#include "civilization/diplomacy/Diplomacy.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string>

namespace StonePotatoes {
namespace Civilization {
namespace Diplomacy {
namespace {

std::string IndexedName(const char* prefix, std::size_t observer,
                        std::size_t target) {
    return std::string(prefix) + std::to_string(observer) + "_to_" +
           std::to_string(target);
}

} // namespace

std::string RelationStockName(std::size_t observerFaction,
                              std::size_t targetFaction) {
    return IndexedName("diplomacy_relation_", observerFaction, targetFaction);
}

std::string MonthlyRelationChangeParameterName(std::size_t observerFaction,
                                               std::size_t targetFaction) {
    return IndexedName("diplomacy_monthly_change_", observerFaction, targetFaction);
}

Economic::SDModel BuildDiplomacyModel(const DiplomacyConfig& config) {
    const std::size_t factionCount = config.initialRelationByFaction.size();
    if (factionCount < 2 || config.monthlyRelationChangeByFaction.size() !=
                                factionCount) {
        throw std::invalid_argument(
            "diplomacy requires at least two factions and matching matrices");
    }

    for (std::size_t observer = 0; observer < factionCount; ++observer) {
        if (config.initialRelationByFaction[observer].size() != factionCount ||
            config.monthlyRelationChangeByFaction[observer].size() !=
                factionCount) {
            throw std::invalid_argument(
                "diplomacy relation and change matrices must be square");
        }
        for (std::size_t target = 0; target < factionCount; ++target) {
            const float relation =
                config.initialRelationByFaction[observer][target];
            const float change =
                config.monthlyRelationChangeByFaction[observer][target];
            if (!std::isfinite(relation) || relation < -1.0f ||
                relation > 1.0f || !std::isfinite(change) ||
                (observer == target && (relation != 0.0f || change != 0.0f))) {
                throw std::invalid_argument(
                    "diplomacy values must be finite, bounded, and exclude self-relations");
            }
        }
    }

    Economic::SDModel model;
    for (std::size_t observer = 0; observer < factionCount; ++observer) {
        for (std::size_t target = 0; target < factionCount; ++target) {
            if (observer == target) continue;
            const std::string stock = RelationStockName(observer, target);
            const std::string changeParameter =
                MonthlyRelationChangeParameterName(observer, target);
            model.AddStock(stock,
                           config.initialRelationByFaction[observer][target],
                           -1.0f, 1.0f);
            model.SetParam(changeParameter,
                           config.monthlyRelationChangeByFaction[observer][target]);
            model.AddFlow(stock, [changeParameter](const Economic::SDState& state) {
                const float change = state.P(changeParameter);
                if (!std::isfinite(change)) return 0.0f;
                return std::clamp(change, -2.0f, 2.0f);
            });
        }
    }
    return model;
}

} // namespace Diplomacy
} // namespace Civilization
} // namespace StonePotatoes
