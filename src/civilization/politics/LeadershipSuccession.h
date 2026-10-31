// SPDX-License-Identifier: MIT
// Copyright (c) 2026 ArchaonHW
#pragma once

#include "civilization/politics/PluralityElection.h"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace StonePotatoes {
namespace Civilization {
namespace Politics {

enum class SuccessionMethod {
    Election,
    Hereditary,
    Appointment,
};

struct LeadershipState {
    std::string officeHolderId;
    std::uint64_t termNumber = 1;
};

/** Inputs supplied by the game's election, family, or appointment rules. */
struct LeadershipSuccessionRequest {
    SuccessionMethod method = SuccessionMethod::Election;
    std::optional<PluralityElectionResult> electionResult;
    std::optional<std::string> designatedSuccessorId;
};

/** An unresolved election tie leaves the incumbent state intact. */
struct LeadershipSuccessionResult {
    LeadershipState state;
    bool resolved = false;
    std::vector<std::string> tiedCandidates;
};

/**
 * Resolve one succession event. This function does not schedule terms or
 * choose heirs/appointed leaders; those policies are supplied by the caller.
 */
LeadershipSuccessionResult ResolveLeadershipSuccession(
    const LeadershipState& current,
    const LeadershipSuccessionRequest& request);

} // namespace Politics
} // namespace Civilization
} // namespace StonePotatoes
