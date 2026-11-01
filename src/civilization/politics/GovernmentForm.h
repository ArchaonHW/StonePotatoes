// SPDX-License-Identifier: MIT
// Copyright (c) 2026 ArchaonHW
#pragma once

#include "civilization/politics/LeadershipSuccession.h"

#include <cstdint>
#include <optional>

namespace StonePotatoes {
namespace Civilization {
namespace Politics {

/** Generic succession forms; voter and appointment policies remain external. */
enum class GovernmentForm {
    Electoral,
    Hereditary,
    Appointed,
};

struct GovernmentFormConfig {
    GovernmentForm form = GovernmentForm::Electoral;
    /** Zero means no fixed term for forms that permit it. */
    std::uint64_t leadershipTermMonths = 0;
};

struct GovernmentRules {
    GovernmentForm form = GovernmentForm::Electoral;
    SuccessionMethod successionMethod = SuccessionMethod::Election;
    std::optional<std::uint64_t> leadershipTermMonths;
};

/**
 * Convert a generic government form into succession and term rules.
 * Electoral forms require a positive term; hereditary forms have no fixed
 * term; appointed forms may have a positive term or be indefinite.
 */
GovernmentRules BuildGovernmentRules(const GovernmentFormConfig& config);

} // namespace Politics
} // namespace Civilization
} // namespace StonePotatoes
