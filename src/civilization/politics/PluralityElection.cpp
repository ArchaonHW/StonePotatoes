// SPDX-License-Identifier: MIT
// Copyright (c) 2026 ArchaonHW
#include "PluralityElection.h"

#include <algorithm>
#include <limits>
#include <set>
#include <stdexcept>

namespace StonePotatoes {
namespace Civilization {
namespace Politics {

PluralityElectionResult TallyPluralityElection(
    const std::vector<ElectionCandidateVotes>& candidates) {
    if (candidates.empty()) {
        throw std::invalid_argument(
            "plurality election requires at least one candidate");
    }

    PluralityElectionResult result;
    std::set<std::string> candidateIds;
    for (const auto& candidate : candidates) {
        if (candidate.candidateId.empty() ||
            !candidateIds.insert(candidate.candidateId).second) {
            throw std::invalid_argument(
                "election candidate IDs must be nonempty and unique");
        }
        if (candidate.votes >
            std::numeric_limits<std::uint64_t>::max() - result.totalVotes) {
            throw std::overflow_error("election vote total exceeds uint64 range");
        }
        result.totalVotes += candidate.votes;
        if (candidate.votes > result.winningVotes) {
            result.winningVotes = candidate.votes;
            result.tiedCandidates.assign(1, candidate.candidateId);
        } else if (candidate.votes == result.winningVotes) {
            result.tiedCandidates.push_back(candidate.candidateId);
        }
    }

    if (result.totalVotes == 0) {
        throw std::invalid_argument("plurality election must contain votes");
    }
    if (result.tiedCandidates.size() == 1) {
        result.winnerCandidateId = result.tiedCandidates.front();
        result.tiedCandidates.clear();
    } else {
        std::sort(result.tiedCandidates.begin(), result.tiedCandidates.end());
    }
    return result;
}

} // namespace Politics
} // namespace Civilization
} // namespace StonePotatoes
