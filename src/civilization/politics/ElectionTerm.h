// SPDX-License-Identifier: MIT
// Copyright (c) 2026 ArchaonHW
#pragma once

#include <cstdint>

namespace StonePotatoes {
namespace Civilization {
namespace Politics {

struct ElectionTermConfig {
    std::uint64_t termLengthMonths = 48;
};

/** Caller-owned election clock; a due election remains due until reset. */
struct ElectionTermState {
    std::uint64_t termLengthMonths = 0;
    std::uint64_t monthsServed = 0;
    std::uint64_t monthsPastDue = 0;
    bool electionDue = false;
};

/** Start a new term clock after a leadership transition. */
ElectionTermState BeginElectionTerm(const ElectionTermConfig& config);

/** Advance the term clock, preserving overdue months until a new term begins. */
ElectionTermState AdvanceElectionTerm(const ElectionTermState& state,
                                      std::uint64_t elapsedMonths);

} // namespace Politics
} // namespace Civilization
} // namespace StonePotatoes
