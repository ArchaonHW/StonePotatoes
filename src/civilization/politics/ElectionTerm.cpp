// SPDX-License-Identifier: MIT
// Copyright (c) 2026 ArchaonHW
#include "ElectionTerm.h"

#include <limits>
#include <stdexcept>

namespace StonePotatoes {
namespace Civilization {
namespace Politics {

ElectionTermState BeginElectionTerm(const ElectionTermConfig& config) {
    if (config.termLengthMonths == 0) {
        throw std::invalid_argument("election term length must be positive");
    }
    return {config.termLengthMonths, 0, 0, false};
}

ElectionTermState AdvanceElectionTerm(const ElectionTermState& state,
                                      std::uint64_t elapsedMonths) {
    if (state.termLengthMonths == 0 ||
        state.monthsServed > state.termLengthMonths || elapsedMonths == 0 ||
        state.electionDue !=
            (state.monthsServed == state.termLengthMonths) ||
        (!state.electionDue && state.monthsPastDue != 0)) {
        throw std::invalid_argument("invalid election term state or elapsed time");
    }

    ElectionTermState next = state;
    if (state.electionDue) {
        if (elapsedMonths >
            std::numeric_limits<std::uint64_t>::max() - state.monthsPastDue) {
            throw std::overflow_error("election overdue duration exceeds uint64 range");
        }
        next.monthsPastDue += elapsedMonths;
        return next;
    }

    const std::uint64_t remaining = state.termLengthMonths - state.monthsServed;
    if (elapsedMonths >= remaining) {
        next.monthsServed = state.termLengthMonths;
        next.electionDue = true;
        next.monthsPastDue = elapsedMonths - remaining;
    } else {
        next.monthsServed += elapsedMonths;
    }
    return next;
}

} // namespace Politics
} // namespace Civilization
} // namespace StonePotatoes
