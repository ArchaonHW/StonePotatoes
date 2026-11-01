// SPDX-License-Identifier: MIT
// Copyright (c) 2026 ArchaonHW
#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace StonePotatoes {
namespace Civilization {
namespace Politics {

struct ElectionVoter {
    std::string voterId;
    std::uint32_t ageYears = 0;
    bool isCitizen = false;
};

struct VoterEligibilityRules {
    std::uint32_t minimumVotingAgeYears = 18;
    bool requireCitizenship = true;
};

/**
 * Select voters using caller-provided age and citizenship data. IDs must be
 * nonempty and unique. Results are sorted by ID for deterministic tally input.
 */
std::vector<std::string> DetermineEligibleVoters(
    const VoterEligibilityRules& rules,
    const std::vector<ElectionVoter>& voters);

} // namespace Politics
} // namespace Civilization
} // namespace StonePotatoes
