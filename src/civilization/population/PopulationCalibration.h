// SPDX-License-Identifier: MIT
// Copyright (c) 2026 ArchaonHW
#pragma once

#include "civilization/population/PopulationDynamics.h"

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

/** One migration-free annual census, indexed by age group. */
struct AgeStructuredCensusObservation {
    int year = 0;
    std::vector<float> populationByAgeGroup;
};

/**
 * Fit an annual age-cohort model to consecutive census years. Age transitions
 * estimate survival rates; fertilityWeights describe relative fertility by
 * age group, and one shared fertility scale is fitted to births. The data must
 * represent years without net migration or carrying-capacity-limited births.
 */
AgeStructuredPopulationConfig CalibrateAgeStructuredPopulation(
    const std::vector<AgeStructuredCensusObservation>& observations,
    const std::vector<float>& fertilityWeights,
    float carryingCapacity);

} // namespace Population
} // namespace Civilization
} // namespace StonePotatoes
