// SPDX-License-Identifier: MIT
// Copyright (c) 2026 ArchaonHW
#include "PluralityElection.h"

#include <algorithm>
#include <limits>
#include <map>
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

PluralityElectionResult TallyPluralityBallots(
    const std::vector<std::string>& candidateIds,
    const std::vector<std::string>& eligibleVoterIds,
    const std::vector<ElectionBallot>& ballots) {
    if (candidateIds.empty()) {
        throw std::invalid_argument(
            "plurality election requires at least one candidate");
    }

    std::set<std::string> candidates;
    for (const auto& candidateId : candidateIds) {
        if (candidateId.empty() || !candidates.insert(candidateId).second) {
            throw std::invalid_argument(
                "election candidate IDs must be nonempty and unique");
        }
    }

    std::set<std::string> eligibleVoters;
    for (const auto& voterId : eligibleVoterIds) {
        if (voterId.empty() || !eligibleVoters.insert(voterId).second) {
            throw std::invalid_argument(
                "eligible voter IDs must be nonempty and unique");
        }
    }

    std::set<std::string> votersWhoCastBallots;
    std::map<std::string, std::uint64_t> voteCounts;
    for (const auto& candidateId : candidateIds) {
        voteCounts.emplace(candidateId, 0);
    }
    for (const auto& ballot : ballots) {
        if (ballot.voterId.empty() ||
            !eligibleVoters.count(ballot.voterId)) {
            throw std::invalid_argument(
                "ballot voter is empty or not eligible");
        }
        if (!votersWhoCastBallots.insert(ballot.voterId).second) {
            throw std::invalid_argument(
                "eligible voter may cast at most one ballot");
        }
        const auto candidate = voteCounts.find(ballot.candidateId);
        if (candidate == voteCounts.end()) {
            throw std::invalid_argument(
                "ballot must name a declared candidate");
        }
        if (candidate->second == std::numeric_limits<std::uint64_t>::max()) {
            throw std::overflow_error("candidate vote count exceeds uint64 range");
        }
        ++candidate->second;
    }

    std::vector<ElectionCandidateVotes> totals;
    totals.reserve(candidateIds.size());
    for (const auto& candidateId : candidateIds) {
        totals.push_back({candidateId, voteCounts.at(candidateId)});
    }
    return TallyPluralityElection(totals);
}

} // namespace Politics
} // namespace Civilization
} // namespace StonePotatoes
