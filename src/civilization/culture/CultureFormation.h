// SPDX-License-Identifier: MIT
// Copyright (c) 2026 ArchaonHW
#pragma once

#include "civilization/economic/SystemDynamics.h"

#include <cstddef>
#include <string>
#include <vector>

namespace StonePotatoes {
namespace Civilization {
namespace Culture {

/** Configurable bounded adoption and fading rates for normalized traits. */
struct CultureFormationConfig {
    // Region-by-trait values in [0, 1]. Trait meanings are caller-defined.
    std::vector<std::vector<float>> initialTraitsByRegion;
    // Fraction of the remaining headroom adopted in one monthly step.
    std::vector<std::vector<float>> monthlyInnovationRateByRegion;
    // Fraction of the current trait value faded in one monthly step.
    std::vector<std::vector<float>> monthlyDecayRateByRegion;
};

/** Build bounded, deterministic local trait formation for each region. */
Economic::SDModel BuildCultureFormationModel(
    const CultureFormationConfig& config);

std::string CultureInnovationRateParameterName(std::size_t regionIndex,
                                              std::size_t traitIndex);
std::string CultureDecayRateParameterName(std::size_t regionIndex,
                                          std::size_t traitIndex);

} // namespace Culture
} // namespace Civilization
} // namespace StonePotatoes
