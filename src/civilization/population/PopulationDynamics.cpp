// SPDX-License-Identifier: MIT
// Copyright (c) 2026 ArchaonHW
#include "PopulationDynamics.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace StonePotatoes {
namespace Civilization {
namespace Population {
namespace {
constexpr char kPopulationStock[] = "population";
constexpr char kGrowthRateParameter[] = "intrinsic_growth_rate_per_month";
constexpr char kNetMigrationParameter[] = "net_migration_per_month";
constexpr char kBirthRateParameter[] = "birth_rate_per_month";
constexpr char kDeathRateParameter[] = "death_rate_per_month";

std::string IndexedName(const char* prefix, std::size_t index) {
    return std::string(prefix) + std::to_string(index);
}

std::vector<float> ProjectedAgeGroupPopulations(
    const Economic::SDState& state,
    const std::vector<std::string>& stockNames,
    const std::vector<std::string>& survivalNames,
    const std::vector<std::string>& birthRateNames,
    float capacity) {
    const std::size_t groupCount = stockNames.size();
    std::vector<float> projected(groupCount, 0.0f);
    float births = 0.0f;
    float survivors = 0.0f;
    for (std::size_t i = 0; i < groupCount; ++i) {
        const float population = state.S(stockNames[i]);
        const float rawSurvival = state.P(survivalNames[i]);
        const float rawBirthRate = state.P(birthRateNames[i]);
        const float survival = std::isfinite(rawSurvival)
            ? std::clamp(rawSurvival, 0.0f, 1.0f) : 0.0f;
        const float birthRate = std::isfinite(rawBirthRate)
            ? std::max(rawBirthRate, 0.0f) : 0.0f;
        births += population * birthRate;
        survivors += population * survival;
        if (i > 0) {
            const float previousRawSurvival = state.P(survivalNames[i - 1]);
            const float previousSurvival = std::isfinite(previousRawSurvival)
                ? std::clamp(previousRawSurvival, 0.0f, 1.0f) : 0.0f;
            projected[i] = state.S(stockNames[i - 1]) * previousSurvival;
        }
    }
    const float acceptedBirths = std::min(
        births, std::max(0.0f, capacity - survivors));
    projected[0] += acceptedBirths;
    projected.back() += state.S(stockNames.back()) *
        (std::isfinite(state.P(survivalNames.back()))
             ? std::clamp(state.P(survivalNames.back()), 0.0f, 1.0f) : 0.0f);
    return projected;
}
} // namespace

std::string AgeGroupStockName(std::size_t index) {
    return IndexedName("population_age_group_", index);
}

std::string AgeGroupSurvivalParameterName(std::size_t index) {
    return IndexedName("survival_rate_per_year_age_group_", index);
}

std::string AgeGroupBirthRateParameterName(std::size_t index) {
    return IndexedName("births_per_person_per_year_age_group_", index);
}

std::string AgeGroupNetMigrationParameterName(std::size_t index) {
    return IndexedName("net_migration_per_year_age_group_", index);
}

Economic::SDModel BuildPopulationModel(const PopulationConfig& config) {
    if (!std::isfinite(config.initialPopulation) ||
        !std::isfinite(config.carryingCapacity) ||
        !std::isfinite(config.intrinsicGrowthRatePerMonth) ||
        !std::isfinite(config.netMigrationPerMonth) ||
        config.carryingCapacity <= 0.0f || config.initialPopulation < 0.0f ||
        config.initialPopulation > config.carryingCapacity) {
        throw std::invalid_argument("invalid population model configuration");
    }

    Economic::SDModel model;
    model.AddStock(kPopulationStock, config.initialPopulation, 0.0f,
                   config.carryingCapacity);
    model.SetParam(kGrowthRateParameter,
                   config.intrinsicGrowthRatePerMonth);
    model.SetParam(kNetMigrationParameter, config.netMigrationPerMonth);
    model.AddFlow(kPopulationStock, [capacity = config.carryingCapacity](
        const Economic::SDState& state) {
        const float population = state.S("population");
        const float densityFactor =
            std::max(0.0f, 1.0f - population / capacity);
        return state.P("intrinsic_growth_rate_per_month") * population *
                   densityFactor +
               state.P("net_migration_per_month");
    });
    return model;
}

Economic::SDModel BuildDemographicPopulationModel(
    const DemographicPopulationConfig& config) {
    if (!std::isfinite(config.initialPopulation) ||
        !std::isfinite(config.carryingCapacity) ||
        !std::isfinite(config.birthRatePerMonth) ||
        !std::isfinite(config.deathRatePerMonth) ||
        !std::isfinite(config.netMigrationPerMonth) ||
        config.carryingCapacity <= 0.0f || config.initialPopulation < 0.0f ||
        config.initialPopulation > config.carryingCapacity ||
        config.birthRatePerMonth < 0.0f || config.deathRatePerMonth < 0.0f) {
        throw std::invalid_argument(
            "invalid demographic population model configuration");
    }

    Economic::SDModel model;
    model.AddStock(kPopulationStock, config.initialPopulation, 0.0f,
                   config.carryingCapacity);
    model.SetParam(kBirthRateParameter, config.birthRatePerMonth);
    model.SetParam(kDeathRateParameter, config.deathRatePerMonth);
    model.SetParam(kNetMigrationParameter, config.netMigrationPerMonth);

    model.AddFlow(kPopulationStock, [capacity = config.carryingCapacity](
        const Economic::SDState& state) {
        const float population = state.S(kPopulationStock);
        const float densityFactor =
            std::max(0.0f, 1.0f - population / capacity);
        return state.P(kBirthRateParameter) * population * densityFactor;
    });
    model.AddFlow(kPopulationStock, [](const Economic::SDState& state) {
        return -state.P(kDeathRateParameter) * state.S(kPopulationStock);
    });
    model.AddFlow(kPopulationStock, [](const Economic::SDState& state) {
        return state.P(kNetMigrationParameter);
    });
    return model;
}

Economic::SDModel BuildAgeStructuredPopulationModel(
    const AgeStructuredPopulationConfig& config) {
    const std::size_t groupCount = config.initialPopulationByAgeGroup.size();
    if (groupCount == 0 ||
        config.annualSurvivalRateByAgeGroup.size() != groupCount ||
        config.annualBirthsPerPersonByAgeGroup.size() != groupCount ||
        (!config.annualNetMigrationByAgeGroup.empty() &&
         config.annualNetMigrationByAgeGroup.size() != groupCount) ||
        !std::isfinite(config.carryingCapacity) ||
        config.carryingCapacity <= 0.0f) {
        throw std::invalid_argument(
            "invalid age-structured population configuration");
    }

    float initialTotal = 0.0f;
    for (std::size_t i = 0; i < groupCount; ++i) {
        const float initial = config.initialPopulationByAgeGroup[i];
        const float survival = config.annualSurvivalRateByAgeGroup[i];
        const float births = config.annualBirthsPerPersonByAgeGroup[i];
        const float migration = config.annualNetMigrationByAgeGroup.empty()
            ? 0.0f : config.annualNetMigrationByAgeGroup[i];
        if (!std::isfinite(initial) || initial < 0.0f ||
            !std::isfinite(survival) || survival < 0.0f || survival > 1.0f ||
            !std::isfinite(births) || births < 0.0f ||
            !std::isfinite(migration)) {
            throw std::invalid_argument(
                "invalid age-group population or rate");
        }
        initialTotal += initial;
    }
    if (!std::isfinite(initialTotal) ||
        initialTotal > config.carryingCapacity) {
        throw std::invalid_argument(
            "initial age-structured population exceeds carrying capacity");
    }

    Economic::SDModel model;
    model.SetDt(1.0f); // rates and cohort transitions are annual
    for (std::size_t i = 0; i < groupCount; ++i) {
        const std::string stock = AgeGroupStockName(i);
        model.AddStock(stock, config.initialPopulationByAgeGroup[i]);
        model.SetParam(AgeGroupSurvivalParameterName(i),
                       config.annualSurvivalRateByAgeGroup[i]);
        model.SetParam(AgeGroupBirthRateParameterName(i),
                       config.annualBirthsPerPersonByAgeGroup[i]);
        model.SetParam(AgeGroupNetMigrationParameterName(i),
                       config.annualNetMigrationByAgeGroup.empty()
                           ? 0.0f : config.annualNetMigrationByAgeGroup[i]);
    }

    std::vector<std::string> stockNames;
    std::vector<std::string> survivalNames;
    std::vector<std::string> birthRateNames;
    std::vector<std::string> migrationNames;
    stockNames.reserve(groupCount);
    survivalNames.reserve(groupCount);
    birthRateNames.reserve(groupCount);
    migrationNames.reserve(groupCount);
    for (std::size_t i = 0; i < groupCount; ++i) {
        stockNames.push_back(AgeGroupStockName(i));
        survivalNames.push_back(AgeGroupSurvivalParameterName(i));
        birthRateNames.push_back(AgeGroupBirthRateParameterName(i));
        migrationNames.push_back(AgeGroupNetMigrationParameterName(i));
    }

    for (std::size_t i = 0; i < groupCount; ++i) {
        if (i + 1 < groupCount) {
            model.AddFlow(stockNames[i], [stock = stockNames[i]](
                const Economic::SDState& state) {
                return -state.S(stock);
            });
            model.AddFlow(stockNames[i + 1], [stock = stockNames[i],
                survival = survivalNames[i]](const Economic::SDState& state) {
                const float rate = state.P(survival);
                const float boundedRate = std::isfinite(rate)
                    ? std::clamp(rate, 0.0f, 1.0f) : 0.0f;
                return state.S(stock) * boundedRate;
            });
        } else {
            model.AddFlow(stockNames[i], [stock = stockNames[i],
                survival = survivalNames[i]](const Economic::SDState& state) {
                const float rate = state.P(survival);
                const float boundedRate = std::isfinite(rate)
                    ? std::clamp(rate, 0.0f, 1.0f) : 0.0f;
                return state.S(stock) * (boundedRate - 1.0f);
            });
        }
    }

    const std::string firstGroup = stockNames.front();
    model.AddFlow(firstGroup, [stockNames, survivalNames, birthRateNames,
                               capacity = config.carryingCapacity](
        const Economic::SDState& state) {
        float projectedSurvivors = 0.0f;
        float births = 0.0f;
        for (std::size_t i = 0; i < stockNames.size(); ++i) {
            const float population = state.S(stockNames[i]);
            const float rawSurvival = state.P(survivalNames[i]);
            const float rawBirthRate = state.P(birthRateNames[i]);
            const float survival = std::isfinite(rawSurvival)
                ? std::clamp(rawSurvival, 0.0f, 1.0f) : 0.0f;
            const float birthRate = std::isfinite(rawBirthRate)
                ? std::max(rawBirthRate, 0.0f) : 0.0f;
            projectedSurvivors += population * survival;
            births += population * birthRate;
        }
        return std::min(births,
                        std::max(0.0f, capacity - projectedSurvivors));
    });

    // Migration is applied after survival, cohort aging, and births. Outflows
    // cannot remove more than the post-aging cohort population. Inflows share
    // the capacity remaining after projected survivors, births and outflows.
    for (std::size_t i = 0; i < groupCount; ++i) {
        model.AddFlow(stockNames[i], [i, stockNames, survivalNames,
            birthRateNames, migrationNames,
            capacity = config.carryingCapacity](const Economic::SDState& state) {
            const float requested = state.P(migrationNames[i]);
            if (!std::isfinite(requested)) return 0.0f;
            const auto projected = ProjectedAgeGroupPopulations(
                state, stockNames, survivalNames, birthRateNames, capacity);
            if (requested < 0.0f) {
                return -std::min(-requested, projected[i]);
            }

            float projectedPopulation = 0.0f;
            float cappedOutflows = 0.0f;
            float requestedImmigration = 0.0f;
            for (std::size_t group = 0; group < stockNames.size(); ++group) {
                const float rawMigration = state.P(migrationNames[group]);
                if (std::isfinite(rawMigration) && rawMigration < 0.0f) {
                    cappedOutflows += std::min(
                        -rawMigration, projected[group]);
                }
                requestedImmigration += std::max(
                    0.0f, std::isfinite(rawMigration) ? rawMigration : 0.0f);
                projectedPopulation += projected[group];
            }
            projectedPopulation -= cappedOutflows;
            if (requestedImmigration <= 0.0f) return 0.0f;
            const float acceptedImmigration = std::min(
                requestedImmigration,
                std::max(0.0f, capacity - projectedPopulation));
            return acceptedImmigration * requested / requestedImmigration;
        });
    }
    return model;
}

} // namespace Population
} // namespace Civilization
} // namespace StonePotatoes
