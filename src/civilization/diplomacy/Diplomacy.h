// SPDX-License-Identifier: MIT
// Copyright (c) 2026 ArchaonHW
#pragma once

#include "civilization/economic/SystemDynamics.h"

#include <cstddef>
#include <string>
#include <vector>

namespace StonePotatoes {
namespace Civilization {
namespace Diplomacy {

/** Configuration for monthly, directed faction-to-faction relations. */
struct DiplomacyConfig {
    // Both matrices are square and indexed [observer faction][target faction].
    // Relation scores are normalized to [-1, 1]; self-relations must be zero.
    std::vector<std::vector<float>> initialRelationByFaction;
    // Signed monthly score change. Positive improves the observer's attitude.
    std::vector<std::vector<float>> monthlyRelationChangeByFaction;
};

/** Build a deterministic monthly relation model; steps default to one month. */
Economic::SDModel BuildDiplomacyModel(const DiplomacyConfig& config);

std::string RelationStockName(std::size_t observerFaction,
                              std::size_t targetFaction);
std::string MonthlyRelationChangeParameterName(std::size_t observerFaction,
                                               std::size_t targetFaction);

} // namespace Diplomacy
} // namespace Civilization
} // namespace StonePotatoes
