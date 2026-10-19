// SPDX-License-Identifier: MIT
// Copyright (c) 2026 ArchaonHW
#pragma once

#include "civilization/economic/SystemDynamics.h"

#include <cstddef>
#include <string>
#include <vector>

namespace StonePotatoes {
namespace Civilization {
namespace Population {

/** Configuration for a single-region logistic population stock. */
struct PopulationConfig {
    float initialPopulation = 1000.0f;
    float carryingCapacity = 10000.0f;
    // Intrinsic growth per month; positive grows and negative declines.
    float intrinsicGrowthRatePerMonth = 0.01f;
    // Net arrivals per month; positive is immigration, negative is emigration.
    float netMigrationPerMonth = 0.0f;
};

/** Configuration for a single-region model with explicit demographic rates. */
struct DemographicPopulationConfig {
    float initialPopulation = 1000.0f;
    float carryingCapacity = 10000.0f;
    // Per-person monthly rates. Births decrease near carrying capacity;
    // deaths apply to the current population.
    float birthRatePerMonth = 0.01f;
    float deathRatePerMonth = 0.005f;
    // Net arrivals per month; positive is immigration, negative is emigration.
    float netMigrationPerMonth = 0.0f;
};

/**
 * One-year age-cohort projection configuration. Rates are annual; each model
 * step advances one age group. The final group is open-ended. Birth rates are
 * births per person in the group and include any fertility-share adjustment.
 */
struct AgeStructuredPopulationConfig {
    std::vector<float> initialPopulationByAgeGroup;
    std::vector<float> annualSurvivalRateByAgeGroup;
    std::vector<float> annualBirthsPerPersonByAgeGroup;
    // Net arrivals per year by age group; positive is immigration. Empty means zero.
    std::vector<float> annualNetMigrationByAgeGroup;
    float carryingCapacity = 1000000.0f;
};

/**
 * Build a reusable stock-flow model with one `population` stock.
 * The net flow is r * P * (1 - P / K) + net migration, bounded to [0, K].
 * The returned model uses monthly steps by default and exposes its growth rate
 * and net migration as parameters for scenario changes.
 */
Economic::SDModel BuildPopulationModel(const PopulationConfig& config);

/**
 * Build a stock-flow model with separate birth, death, and net migration flows.
 * Births are density-limited by carrying capacity; all updates remain bounded
 * to [0, carryingCapacity]. The monthly rates are runtime-adjustable parameters.
 */
Economic::SDModel BuildDemographicPopulationModel(
    const DemographicPopulationConfig& config);

/**
 * Build a deterministic Leslie-style cohort model. Every step represents a
 * year: surviving members move to the next group, the final group stays open,
 * and births enter group zero. Births are capped by the remaining total
 * carrying capacity. Cohort stock and runtime parameter names are available
 * from the helpers below.
 */
Economic::SDModel BuildAgeStructuredPopulationModel(
    const AgeStructuredPopulationConfig& config);
std::string AgeGroupStockName(std::size_t index);
std::string AgeGroupSurvivalParameterName(std::size_t index);
std::string AgeGroupBirthRateParameterName(std::size_t index);
std::string AgeGroupNetMigrationParameterName(std::size_t index);

} // namespace Population
} // namespace Civilization
} // namespace StonePotatoes
