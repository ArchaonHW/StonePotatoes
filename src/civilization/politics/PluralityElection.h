// SPDX-License-Identifier: MIT
// Copyright (c) 2026 ArchaonHW
#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace StonePotatoes {
namespace Civilization {
namespace Politics {

/** Votes recorded for one candidate in a plurality election. */
struct ElectionCandidateVotes {
    std::string candidateId;
    std::uint64_t votes = 0;
};

/** One ballot cast by an eligible voter for a declared candidate. */
struct ElectionBallot {
    std::string voterId;
    std::string candidateId;
};

/** Deterministic result; a tie has no winner and lists all tied candidates. */
struct PluralityElectionResult {
    std::optional<std::string> winnerCandidateId;
    std::uint64_t winningVotes = 0;
    std::uint64_t totalVotes = 0;
    std::vector<std::string> tiedCandidates;
};

/**
 * Tally supplied vote counts using the plurality rule. The caller owns
 * eligibility, ballots, districts, and any political tie-breaking procedure.
 */
PluralityElectionResult TallyPluralityElection(
    const std::vector<ElectionCandidateVotes>& candidates);

/**
 * Validate and tally individual ballots. Each eligible voter may cast at most
 * one ballot, and each ballot must name a declared candidate. The caller owns
 * voter registration, district rules, and ballot collection.
 */
PluralityElectionResult TallyPluralityBallots(
    const std::vector<std::string>& candidateIds,
    const std::vector<std::string>& eligibleVoterIds,
    const std::vector<ElectionBallot>& ballots);

} // namespace Politics
} // namespace Civilization
} // namespace StonePotatoes
