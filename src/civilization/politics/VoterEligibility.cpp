// SPDX-License-Identifier: MIT
// Copyright (c) 2026 ArchaonHW
#include "VoterEligibility.h"

#include <algorithm>
#include <set>
#include <stdexcept>

namespace StonePotatoes {
namespace Civilization {
namespace Politics {

std::vector<std::string> DetermineEligibleVoters(
    const VoterEligibilityRules& rules,
    const std::vector<ElectionVoter>& voters) {
    std::set<std::string> seenIds;
    std::vector<std::string> eligibleIds;
    for (const auto& voter : voters) {
        if (voter.voterId.empty() || !seenIds.insert(voter.voterId).second) {
            throw std::invalid_argument(
                "election voter IDs must be nonempty and unique");
        }
        if (voter.ageYears >= rules.minimumVotingAgeYears &&
            (!rules.requireCitizenship || voter.isCitizen)) {
            eligibleIds.push_back(voter.voterId);
        }
    }
    std::sort(eligibleIds.begin(), eligibleIds.end());
    return eligibleIds;
}

} // namespace Politics
} // namespace Civilization
} // namespace StonePotatoes
