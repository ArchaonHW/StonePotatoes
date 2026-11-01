// SPDX-License-Identifier: MIT
// Copyright (c) 2026 ArchaonHW
#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace StonePotatoes {
namespace Civilization {
namespace Politics {

struct ElectionCandidateProfile {
    std::string candidateId;
    std::uint32_t ageYears = 0;
    bool isCitizen = false;
};

struct CandidateEligibilityRules {
    std::uint32_t minimumCandidateAgeYears = 21;
    bool requireCitizenship = true;
};

/**
 * Select declared candidates using caller-provided age and citizenship data.
 * IDs must be nonempty and unique; results are sorted by ID.
 */
std::vector<std::string> DetermineEligibleCandidates(
    const CandidateEligibilityRules& rules,
    const std::vector<ElectionCandidateProfile>& candidates);

} // namespace Politics
} // namespace Civilization
} // namespace StonePotatoes
