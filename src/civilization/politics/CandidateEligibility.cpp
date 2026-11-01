// SPDX-License-Identifier: MIT
// Copyright (c) 2026 ArchaonHW
#include "CandidateEligibility.h"

#include <algorithm>
#include <set>
#include <stdexcept>

namespace StonePotatoes {
namespace Civilization {
namespace Politics {

std::vector<std::string> DetermineEligibleCandidates(
    const CandidateEligibilityRules& rules,
    const std::vector<ElectionCandidateProfile>& candidates) {
    std::set<std::string> seenIds;
    std::vector<std::string> eligibleIds;
    for (const auto& candidate : candidates) {
        if (candidate.candidateId.empty() ||
            !seenIds.insert(candidate.candidateId).second) {
            throw std::invalid_argument(
                "election candidate IDs must be nonempty and unique");
        }
        if (candidate.ageYears >= rules.minimumCandidateAgeYears &&
            (!rules.requireCitizenship || candidate.isCitizen)) {
            eligibleIds.push_back(candidate.candidateId);
        }
    }
    std::sort(eligibleIds.begin(), eligibleIds.end());
    return eligibleIds;
}

} // namespace Politics
} // namespace Civilization
} // namespace StonePotatoes
