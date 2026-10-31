// SPDX-License-Identifier: MIT
// Copyright (c) 2026 ArchaonHW
#include "LeadershipSuccession.h"

#include <limits>
#include <stdexcept>
#include <utility>

namespace StonePotatoes {
namespace Civilization {
namespace Politics {

LeadershipSuccessionResult ResolveLeadershipSuccession(
    const LeadershipState& current,
    const LeadershipSuccessionRequest& request) {
    if (current.officeHolderId.empty() || current.termNumber == 0) {
        throw std::invalid_argument(
            "current leadership requires a holder ID and positive term number");
    }

    std::string successorId;
    switch (request.method) {
    case SuccessionMethod::Election:
        if (!request.electionResult.has_value()) {
            throw std::invalid_argument(
                "election succession requires a plurality election result");
        }
        if (!request.electionResult->winnerCandidateId.has_value()) {
            if (request.electionResult->tiedCandidates.size() < 2) {
                throw std::invalid_argument(
                    "unresolved election must identify at least two tied candidates");
            }
            return {current, false, request.electionResult->tiedCandidates};
        }
        successorId = *request.electionResult->winnerCandidateId;
        break;
    case SuccessionMethod::Hereditary:
    case SuccessionMethod::Appointment:
        if (!request.designatedSuccessorId.has_value()) {
            throw std::invalid_argument(
                "hereditary or appointed succession requires a designated successor");
        }
        successorId = *request.designatedSuccessorId;
        break;
    default:
        throw std::invalid_argument("unknown leadership succession method");
    }

    if (successorId.empty()) {
        throw std::invalid_argument("successor ID must not be empty");
    }
    if (current.termNumber == std::numeric_limits<std::uint64_t>::max()) {
        throw std::overflow_error("leadership term number exceeds uint64 range");
    }
    return {{std::move(successorId), current.termNumber + 1}, true, {}};
}

} // namespace Politics
} // namespace Civilization
} // namespace StonePotatoes
