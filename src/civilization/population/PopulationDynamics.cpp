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

struct RegionalStepProjection {
    std::vector<float> populationAfterGrowth;
    std::vector<float> migrationDelta;
};

RegionalStepProjection ProjectRegionalStep(
    const Economic::SDState& state,
    const std::vector<std::string>& stockNames,
    const std::vector<std::string>& growthParameterNames,
    const std::vector<float>& capacities,
    const std::vector<std::vector<std::string>>& migrationParameterNames) {
    const std::size_t regionCount = stockNames.size();
    RegionalStepProjection result{
        std::vector<float>(regionCount, 0.0f),
        std::vector<float>(regionCount, 0.0f)};
    std::vector<std::vector<float>> requestedTransfers(
        regionCount, std::vector<float>(regionCount, 0.0f));

    for (std::size_t source = 0; source < regionCount; ++source) {
        const float population = state.S(stockNames[source]);
        const float rawGrowthRate = state.P(growthParameterNames[source]);
        const float growthRate = std::isfinite(rawGrowthRate)
            ? rawGrowthRate : 0.0f;
        const float densityFactor =
            std::max(0.0f, 1.0f - population / capacities[source]);
        result.populationAfterGrowth[source] = std::clamp(
            population + growthRate * population * densityFactor,
            0.0f, capacities[source]);
    }

    for (std::size_t source = 0; source < regionCount; ++source) {
        float requestedOutflow = 0.0f;
        for (std::size_t destination = 0; destination < regionCount;
             ++destination) {
            const float rawRequest = state.P(
                migrationParameterNames[source][destination]);
            const float request = source != destination &&
                    std::isfinite(rawRequest)
                ? std::max(0.0f, rawRequest) : 0.0f;
            requestedTransfers[source][destination] = request;
            requestedOutflow += request;
        }
        if (requestedOutflow > result.populationAfterGrowth[source] &&
            requestedOutflow > 0.0f) {
            const float sourceScale =
                result.populationAfterGrowth[source] / requestedOutflow;
            for (float& request : requestedTransfers[source]) {
                request *= sourceScale;
            }
        }
    }

    std::vector<float> destinationScales(regionCount, 1.0f);
    for (std::size_t destination = 0; destination < regionCount;
         ++destination) {
        float requestedInflow = 0.0f;
        for (std::size_t source = 0; source < regionCount; ++source) {
            requestedInflow += requestedTransfers[source][destination];
        }
        const float availableCapacity = std::max(
            0.0f, capacities[destination] -
                      result.populationAfterGrowth[destination]);
        if (requestedInflow > availableCapacity && requestedInflow > 0.0f) {
            destinationScales[destination] =
                availableCapacity / requestedInflow;
        }
    }

    for (std::size_t source = 0; source < regionCount; ++source) {
        for (std::size_t destination = 0; destination < regionCount;
             ++destination) {
            const float transfer = requestedTransfers[source][destination] *
                destinationScales[destination];
            result.migrationDelta[source] -= transfer;
            result.migrationDelta[destination] += transfer;
        }
    }
    return result;
}

std::vector<std::vector<float>> ProjectRegionalAgeStep(
    const Economic::SDState& state,
    const std::vector<std::vector<std::string>>& stockNames,
    const std::vector<std::string>& survivalNames,
    const std::vector<std::string>& birthNames,
    const std::vector<float>& capacities,
    const std::vector<std::vector<std::vector<std::string>>>& migrationNames) {
    const std::size_t regions = stockNames.size();
    const std::size_t groups = survivalNames.size();
    std::vector<std::vector<float>> projected(
        regions, std::vector<float>(groups, 0.0f));

    // Annual survival and aging happen inside each region. Births enter the
    // first cohort and are limited by that region's remaining capacity.
    for (std::size_t region = 0; region < regions; ++region) {
        float survivors = 0.0f;
        float births = 0.0f;
        for (std::size_t age = 0; age < groups; ++age) {
            const float population = state.S(stockNames[region][age]);
            const float rawSurvival = state.P(survivalNames[age]);
            const float rawBirth = state.P(birthNames[age]);
            const float survival = std::isfinite(rawSurvival)
                ? std::clamp(rawSurvival, 0.0f, 1.0f) : 0.0f;
            const float birthRate = std::isfinite(rawBirth)
                ? std::max(rawBirth, 0.0f) : 0.0f;
            survivors += population * survival;
            births += population * birthRate;
            if (age > 0) {
                const float priorRawSurvival =
                    state.P(survivalNames[age - 1]);
                const float priorSurvival = std::isfinite(priorRawSurvival)
                    ? std::clamp(priorRawSurvival, 0.0f, 1.0f) : 0.0f;
                projected[region][age] =
                    state.S(stockNames[region][age - 1]) * priorSurvival;
            }
        }
        projected[region][0] = std::min(
            births, std::max(0.0f, capacities[region] - survivors));
        projected[region].back() +=
            state.S(stockNames[region].back()) *
            (std::isfinite(state.P(survivalNames.back()))
                 ? std::clamp(state.P(survivalNames.back()), 0.0f, 1.0f)
                 : 0.0f);
    }

    // Bound each source cohort's requested outflow by its post-aging size.
    std::vector<std::vector<std::vector<float>>> transfers(
        regions, std::vector<std::vector<float>>(
            regions, std::vector<float>(groups, 0.0f)));
    for (std::size_t source = 0; source < regions; ++source) {
        for (std::size_t age = 0; age < groups; ++age) {
            float requested = 0.0f;
            for (std::size_t destination = 0; destination < regions;
                 ++destination) {
                const float value = state.P(migrationNames[source][destination][age]);
                const float accepted = source != destination &&
                        std::isfinite(value)
                    ? std::max(0.0f, value) : 0.0f;
                transfers[source][destination][age] = accepted;
                requested += accepted;
            }
            if (requested > projected[source][age] && requested > 0.0f) {
                const float scale = projected[source][age] / requested;
                for (auto& destinationTransfers : transfers[source])
                    destinationTransfers[age] *= scale;
            }
        }
    }

    // Destinations share unused capacity proportionally across incoming
    // regions and cohorts. Compute every scale from the same pre-migration
    // snapshot so region iteration order cannot affect the result.
    std::vector<float> destinationScales(regions, 1.0f);
    for (std::size_t destination = 0; destination < regions; ++destination) {
        float inflow = 0.0f;
        for (std::size_t source = 0; source < regions; ++source) {
            for (std::size_t age = 0; age < groups; ++age) {
                inflow += transfers[source][destination][age];
            }
        }
        float population = 0.0f;
        for (float value : projected[destination]) population += value;
        const float room = std::max(0.0f, capacities[destination] - population);
        if (inflow > room && inflow > 0.0f)
            destinationScales[destination] = room / inflow;
    }
    for (std::size_t destination = 0; destination < regions; ++destination) {
        const float scale = destinationScales[destination];
        for (std::size_t source = 0; source < regions; ++source) {
            for (std::size_t age = 0; age < groups; ++age) {
                const float transfer = transfers[source][destination][age] * scale;
                projected[source][age] -= transfer;
                projected[destination][age] += transfer;
            }
        }
    }
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

std::string RegionPopulationStockName(std::size_t regionIndex) {
    return IndexedName("population_region_", regionIndex);
}

std::string RegionGrowthParameterName(std::size_t regionIndex) {
    return IndexedName("intrinsic_growth_rate_per_month_region_", regionIndex);
}

std::string RegionMigrationParameterName(std::size_t fromRegionIndex,
                                         std::size_t toRegionIndex) {
    return IndexedName("migration_per_month_region_", fromRegionIndex) +
           "_to_" + std::to_string(toRegionIndex);
}

std::string RegionAgeGroupStockName(std::size_t regionIndex,
                                    std::size_t ageGroupIndex) {
    return "population_region_" + std::to_string(regionIndex) +
           "_age_group_" + std::to_string(ageGroupIndex);
}

std::string RegionalAgeMigrationParameterName(std::size_t fromRegionIndex,
                                              std::size_t toRegionIndex,
                                              std::size_t ageGroupIndex) {
    return "migration_per_year_region_" + std::to_string(fromRegionIndex) +
           "_to_" + std::to_string(toRegionIndex) + "_age_group_" +
           std::to_string(ageGroupIndex);
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

Economic::SDModel BuildRegionalPopulationModel(
    const RegionalPopulationConfig& config) {
    const std::size_t regionCount = config.initialPopulationByRegion.size();
    if (regionCount == 0 ||
        config.carryingCapacityByRegion.size() != regionCount ||
        config.intrinsicGrowthRatePerMonthByRegion.size() != regionCount ||
        config.monthlyMigrationFromRegionToRegion.size() != regionCount) {
        throw std::invalid_argument(
            "regional population vectors must have matching nonzero sizes");
    }

    for (std::size_t region = 0; region < regionCount; ++region) {
        const float initial = config.initialPopulationByRegion[region];
        const float capacity = config.carryingCapacityByRegion[region];
        const float growthRate =
            config.intrinsicGrowthRatePerMonthByRegion[region];
        if (!std::isfinite(initial) || !std::isfinite(capacity) ||
            !std::isfinite(growthRate) || capacity <= 0.0f || initial < 0.0f ||
            initial > capacity ||
            config.monthlyMigrationFromRegionToRegion[region].size() !=
                regionCount) {
            throw std::invalid_argument(
                "invalid regional population configuration");
        }
        for (std::size_t destination = 0; destination < regionCount;
             ++destination) {
            const float migration =
                config.monthlyMigrationFromRegionToRegion[region][destination];
            if (!std::isfinite(migration) || migration < 0.0f ||
                (region == destination && migration != 0.0f)) {
                throw std::invalid_argument(
                    "regional migration must be finite, nonnegative, and "
                    "exclude self-transfers");
            }
        }
    }

    std::vector<std::string> stockNames;
    std::vector<std::string> growthParameterNames;
    std::vector<std::vector<std::string>> migrationParameterNames(
        regionCount, std::vector<std::string>(regionCount));
    stockNames.reserve(regionCount);
    growthParameterNames.reserve(regionCount);

    Economic::SDModel model;
    for (std::size_t region = 0; region < regionCount; ++region) {
        const std::string stock = RegionPopulationStockName(region);
        const std::string growth = RegionGrowthParameterName(region);
        stockNames.push_back(stock);
        growthParameterNames.push_back(growth);
        model.AddStock(stock, config.initialPopulationByRegion[region], 0.0f,
                       config.carryingCapacityByRegion[region]);
        model.SetParam(growth,
                       config.intrinsicGrowthRatePerMonthByRegion[region]);
        for (std::size_t destination = 0; destination < regionCount;
             ++destination) {
            const std::string migration =
                RegionMigrationParameterName(region, destination);
            migrationParameterNames[region][destination] = migration;
            model.SetParam(
                migration,
                config.monthlyMigrationFromRegionToRegion[region][destination]);
        }
    }

    for (std::size_t region = 0; region < regionCount; ++region) {
        model.AddFlow(stockNames[region], [region, stockNames,
            growthParameterNames,
            capacities = config.carryingCapacityByRegion,
            migrationParameterNames](const Economic::SDState& state) {
            const RegionalStepProjection projection = ProjectRegionalStep(
                state, stockNames, growthParameterNames, capacities,
                migrationParameterNames);
            return projection.populationAfterGrowth[region] -
                       state.S(stockNames[region]) +
                   projection.migrationDelta[region];
        });
    }
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

Economic::SDModel BuildRegionalAgeStructuredPopulationModel(
    const RegionalAgeStructuredPopulationConfig& config) {
    const std::size_t regions =
        config.initialPopulationByRegionAndAgeGroup.size();
    const std::size_t groups = config.annualSurvivalRateByAgeGroup.size();
    if (regions == 0 || groups == 0 ||
        config.carryingCapacityByRegion.size() != regions ||
        config.annualBirthsPerPersonByAgeGroup.size() != groups ||
        config.annualMigrationFromRegionToRegionByAgeGroup.size() != regions) {
        throw std::invalid_argument(
            "regional age-structured population dimensions must be nonzero and match");
    }

    for (float survival : config.annualSurvivalRateByAgeGroup) {
        if (!std::isfinite(survival) || survival < 0.0f || survival > 1.0f)
            throw std::invalid_argument("annual survival rates must be in [0, 1]");
    }
    for (float birth : config.annualBirthsPerPersonByAgeGroup) {
        if (!std::isfinite(birth) || birth < 0.0f)
            throw std::invalid_argument("annual birth rates must be finite and nonnegative");
    }

    for (std::size_t region = 0; region < regions; ++region) {
        const float capacity = config.carryingCapacityByRegion[region];
        if (!std::isfinite(capacity) || capacity <= 0.0f ||
            config.initialPopulationByRegionAndAgeGroup[region].size() != groups ||
            config.annualMigrationFromRegionToRegionByAgeGroup[region].size() != regions) {
            throw std::invalid_argument("invalid regional age-structured population dimensions");
        }
        float initialTotal = 0.0f;
        for (float initial : config.initialPopulationByRegionAndAgeGroup[region]) {
            if (!std::isfinite(initial) || initial < 0.0f)
                throw std::invalid_argument("initial regional cohort populations must be finite and nonnegative");
            initialTotal += initial;
        }
        if (!std::isfinite(initialTotal) || initialTotal > capacity)
            throw std::invalid_argument("initial regional population exceeds carrying capacity");
        for (std::size_t destination = 0; destination < regions; ++destination) {
            const auto& rates = config.annualMigrationFromRegionToRegionByAgeGroup[region][destination];
            if (rates.size() != groups)
                throw std::invalid_argument("regional migration must provide one rate per age group");
            for (float rate : rates) {
                if (!std::isfinite(rate) || rate < 0.0f ||
                    (region == destination && rate != 0.0f)) {
                    throw std::invalid_argument(
                        "regional migration must be finite, nonnegative, and exclude self-transfers");
                }
            }
        }
    }

    Economic::SDModel model;
    model.SetDt(1.0f);
    std::vector<std::vector<std::string>> stockNames(
        regions, std::vector<std::string>(groups));
    std::vector<std::vector<std::vector<std::string>>> migrationNames(
        regions, std::vector<std::vector<std::string>>(
            regions, std::vector<std::string>(groups)));
    std::vector<std::string> survivalNames(groups);
    std::vector<std::string> birthNames(groups);

    for (std::size_t age = 0; age < groups; ++age) {
        survivalNames[age] = AgeGroupSurvivalParameterName(age);
        birthNames[age] = AgeGroupBirthRateParameterName(age);
        model.SetParam(survivalNames[age], config.annualSurvivalRateByAgeGroup[age]);
        model.SetParam(birthNames[age], config.annualBirthsPerPersonByAgeGroup[age]);
    }
    for (std::size_t region = 0; region < regions; ++region) {
        for (std::size_t age = 0; age < groups; ++age) {
            const std::string stock = RegionAgeGroupStockName(region, age);
            stockNames[region][age] = stock;
            model.AddStock(stock,
                config.initialPopulationByRegionAndAgeGroup[region][age],
                0.0f, config.carryingCapacityByRegion[region]);
        }
        for (std::size_t destination = 0; destination < regions; ++destination) {
            for (std::size_t age = 0; age < groups; ++age) {
                const std::string parameter = RegionalAgeMigrationParameterName(
                    region, destination, age);
                migrationNames[region][destination][age] = parameter;
                model.SetParam(parameter,
                    config.annualMigrationFromRegionToRegionByAgeGroup
                        [region][destination][age]);
            }
        }
    }

    for (std::size_t region = 0; region < regions; ++region) {
        for (std::size_t age = 0; age < groups; ++age) {
            model.AddFlow(stockNames[region][age],
                [region, age, stockNames, survivalNames, birthNames,
                 capacities = config.carryingCapacityByRegion, migrationNames](
                    const Economic::SDState& state) {
                    const auto projected = ProjectRegionalAgeStep(
                        state, stockNames, survivalNames, birthNames,
                        capacities, migrationNames);
                    return projected[region][age] -
                           state.S(stockNames[region][age]);
                });
        }
    }
    return model;
}

} // namespace Population
} // namespace Civilization
} // namespace StonePotatoes
