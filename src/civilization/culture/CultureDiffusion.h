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

/**
 * Group regions by cultural similarity using connected components. Two
 * regions are linked when the Euclidean distance between their normalized
 * trait vectors is at most maxDistance; links are treated as undirected.
 * Returned group ids are stable for a fixed input order, starting at zero.
 */
std::vector<std::size_t> ClusterCultureRegionsBySimilarity(
    const std::vector<std::vector<float>>& traitsByRegion,
    float maxDistance);

/**
 * Track stable culture identity ids while similarity clusters change over
 * time. Regions must stay in the same input order; matching prefers the
 * largest overlap between previous identities and new groups. New groups
 * receive monotonically increasing ids.
 */
class CultureIdentityTracker {
public:
    std::vector<std::size_t> Update(
        const std::vector<std::vector<float>>& traitsByRegion,
        float maxDistance);
    void Reset();

private:
    std::vector<std::size_t> m_PreviousIdentityByRegion;
    std::size_t m_NextIdentityId = 0;
};

} // namespace Culture
} // namespace Civilization
} // namespace StonePotatoes
