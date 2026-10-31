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

/** One annual census, indexed [region][age group]. */
struct RegionalAgeStructuredCensusObservation {
    int year = 0;
    std::vector<std::vector<float>> populationByRegionAndAgeGroup;
};

/** Observed net resident transfers during one annual census interval. */
struct RegionalAgeStructuredMigrationObservation {
    int fromYear = 0;
    // Signed net arrivals after aging and births; rows are [region][age group].
    // Values must conserve population across regions for each age group.
    std::vector<std::vector<float>> netMigrationByRegionAndAgeGroup;
};

/** Shared age-specific annual survival and per-capita birth rates. */
struct RegionalAgeStructuredPopulationRates {
    std::vector<float> annualSurvivalRateByAgeGroup;
    std::vector<float> annualBirthsPerPersonByAgeGroup;
};

/**
 * Fit shared annual demographic rates from multi-region census data when
 * per-region, per-age-group net migration is known for every interval. Net
 * migration is applied after aging and births. This returns rates only; the
 * caller supplies regional capacities and directed migration to the runtime
 * model.
 */
RegionalAgeStructuredPopulationRates
CalibrateRegionalAgeStructuredPopulationRates(
    const std::vector<RegionalAgeStructuredCensusObservation>& observations,
    const std::vector<RegionalAgeStructuredMigrationObservation>& migration,
    const std::vector<float>& fertilityWeights);

} // namespace Population
} // namespace Civilization
} // namespace StonePotatoes
