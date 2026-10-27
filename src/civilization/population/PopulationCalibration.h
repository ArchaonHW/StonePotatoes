// SPDX-License-Identifier: MIT
// Copyright (c) 2026 ArchaonHW
#pragma once

#include <vector>

namespace StonePotatoes {
namespace Civilization {
namespace Population {

/** Population observation at a month offset from the calibration baseline. */
struct LogisticPopulationObservation {
    float month = 0.0f;
    float population = 0.0f;
};

/** Estimated monthly logistic growth rate and population at month zero. */
struct LogisticPopulationCalibration {
    float intrinsicGrowthRatePerMonth = 0.0f;
    float estimatedInitialPopulation = 0.0f;
};

/**
 * Fit a constant-capacity logistic curve to population observations.
 *
 * The carrying capacity must be known and strictly exceed every observation.
 * A least-squares fit is performed on logit(population / carryingCapacity),
 * which is linear in time for the logistic model used by PopulationDynamics.
 * At least two strictly increasing, nonnegative month offsets are required.
 */
LogisticPopulationCalibration CalibrateLogisticGrowth(
    const std::vector<LogisticPopulationObservation>& observations,
    float carryingCapacity);

} // namespace Population
} // namespace Civilization
} // namespace StonePotatoes
