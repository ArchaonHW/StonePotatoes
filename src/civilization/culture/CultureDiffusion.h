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

/** Monthly diffusion for caller-defined, normalized cultural traits. */
struct CultureDiffusionConfig {
    // Rows are regions; each row contains the same caller-defined traits in [0, 1].
    std::vector<std::vector<float>> initialTraitsByRegion;
    // Rows are source regions, columns are destinations. Each value is a
    // monthly assimilation fraction in [0, 1]; incoming values per region
    // must sum to at most 1 to keep the update stable and bounded.
    std::vector<std::vector<float>>
        monthlyAssimilationRateFromRegionToRegion;
};

/** Build synchronous regional trait diffusion over a directed contact graph. */
Economic::SDModel BuildCultureDiffusionModel(
    const CultureDiffusionConfig& config);

std::string CultureTraitStockName(std::size_t regionIndex,
                                  std::size_t traitIndex);
std::string CultureAssimilationParameterName(std::size_t fromRegionIndex,
                                             std::size_t toRegionIndex);

} // namespace Culture
} // namespace Civilization
} // namespace StonePotatoes
