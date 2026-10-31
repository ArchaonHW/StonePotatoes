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

} // namespace Politics
} // namespace Civilization
} // namespace StonePotatoes
