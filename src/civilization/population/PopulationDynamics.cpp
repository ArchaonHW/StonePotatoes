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
        if (!std::isfinite(initial) || initial < 0.0f ||
            !std::isfinite(survival) || survival < 0.0f || survival > 1.0f ||
            !std::isfinite(births) || births < 0.0f) {
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
    }

    std::vector<std::string> stockNames;
    std::vector<std::string> survivalNames;
    std::vector<std::string> birthRateNames;
    stockNames.reserve(groupCount);
    survivalNames.reserve(groupCount);
    birthRateNames.reserve(groupCount);
    for (std::size_t i = 0; i < groupCount; ++i) {
        stockNames.push_back(AgeGroupStockName(i));
        survivalNames.push_back(AgeGroupSurvivalParameterName(i));
        birthRateNames.push_back(AgeGroupBirthRateParameterName(i));
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
    return model;
}

} // namespace Population
} // namespace Civilization
} // namespace StonePotatoes
