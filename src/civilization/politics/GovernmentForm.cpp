// SPDX-License-Identifier: MIT
// Copyright (c) 2026 ArchaonHW
#include "GovernmentForm.h"

#include <stdexcept>

namespace StonePotatoes {
namespace Civilization {
namespace Politics {

GovernmentRules BuildGovernmentRules(const GovernmentFormConfig& config) {
    switch (config.form) {
    case GovernmentForm::Electoral:
        if (config.leadershipTermMonths == 0) {
            throw std::invalid_argument(
                "electoral government requires a positive leadership term");
        }
        return {config.form, SuccessionMethod::Election,
                config.leadershipTermMonths};
    case GovernmentForm::Hereditary:
        if (config.leadershipTermMonths != 0) {
            throw std::invalid_argument(
                "hereditary government does not use a fixed leadership term");
        }
        return {config.form, SuccessionMethod::Hereditary, std::nullopt};
    case GovernmentForm::Appointed:
        return {config.form, SuccessionMethod::Appointment,
                config.leadershipTermMonths == 0
                    ? std::nullopt
                    : std::optional<std::uint64_t>(config.leadershipTermMonths)};
    }
    throw std::invalid_argument("unknown government form");
}

} // namespace Politics
} // namespace Civilization
} // namespace StonePotatoes
