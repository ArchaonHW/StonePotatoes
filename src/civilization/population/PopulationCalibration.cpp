// SPDX-License-Identifier: MIT
// Copyright (c) 2026 ArchaonHW
#include "civilization/population/PopulationCalibration.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>

namespace StonePotatoes {
namespace Civilization {
namespace Population {
namespace {

double InverseLogit(double value) {
    if (value >= 0.0) {
        return 1.0 / (1.0 + std::exp(-value));
    }
    const double expValue = std::exp(value);
    return expValue / (1.0 + expValue);
}

} // namespace

LogisticPopulationCalibration CalibrateLogisticGrowth(
    const std::vector<LogisticPopulationObservation>& observations,
    float carryingCapacity) {
    if (!std::isfinite(carryingCapacity) || carryingCapacity <= 0.0f ||
        observations.size() < 2) {
        throw std::invalid_argument(
            "logistic calibration requires positive capacity and two observations");
    }

    std::vector<double> transformed;
    transformed.reserve(observations.size());
    double timeSum = 0.0;
    double valueSum = 0.0;
    for (std::size_t i = 0; i < observations.size(); ++i) {
        const auto& observation = observations[i];
        if (!std::isfinite(observation.month) || observation.month < 0.0f ||
            !std::isfinite(observation.population) ||
            observation.population <= 0.0f ||
            observation.population >= carryingCapacity ||
            (i > 0 && observation.month <= observations[i - 1].month)) {
            throw std::invalid_argument(
                "logistic calibration observations must be finite, ordered, and below capacity");
        }
        const double population = observation.population;
        const double capacity = carryingCapacity;
        const double logit = std::log(population / (capacity - population));
        transformed.push_back(logit);
        timeSum += observation.month;
        valueSum += logit;
    }

    const double count = static_cast<double>(observations.size());
    const double meanTime = timeSum / count;
    const double meanValue = valueSum / count;
    double covariance = 0.0;
    double timeVariance = 0.0;
    for (std::size_t i = 0; i < observations.size(); ++i) {
        const double centeredTime = observations[i].month - meanTime;
        covariance += centeredTime * (transformed[i] - meanValue);
        timeVariance += centeredTime * centeredTime;
    }
    const double growthRate = covariance / timeVariance;
    const double intercept = meanValue - growthRate * meanTime;
    const double initialPopulation =
        static_cast<double>(carryingCapacity) * InverseLogit(intercept);
    const double floatLimit = std::numeric_limits<float>::max();
    if (!std::isfinite(growthRate) || !std::isfinite(initialPopulation) ||
        std::abs(growthRate) > floatLimit || initialPopulation <= 0.0 ||
        initialPopulation > floatLimit ||
        initialPopulation < std::numeric_limits<float>::denorm_min()) {
        throw std::overflow_error(
            "logistic calibration result is outside the float range");
    }

    return {static_cast<float>(growthRate),
            static_cast<float>(initialPopulation)};
}

AgeStructuredPopulationConfig CalibrateAgeStructuredPopulation(
    const std::vector<AgeStructuredCensusObservation>& observations,
    const std::vector<float>& fertilityWeights,
    float carryingCapacity) {
    if (observations.size() < 2 || fertilityWeights.size() < 2 ||
        !std::isfinite(carryingCapacity) || carryingCapacity <= 0.0f) {
        throw std::invalid_argument(
            "age-structured calibration requires census years, age groups, and positive capacity");
    }
    const std::size_t groupCount = fertilityWeights.size();
    bool hasFertileWeight = false;
    for (float weight : fertilityWeights) {
        if (!std::isfinite(weight) || weight < 0.0f) {
            throw std::invalid_argument(
                "fertility weights must be finite and nonnegative");
        }
        hasFertileWeight = hasFertileWeight || weight > 0.0f;
    }
    if (!hasFertileWeight) {
        throw std::invalid_argument("at least one fertility weight must be positive");
    }

    for (std::size_t year = 0; year < observations.size(); ++year) {
        const auto& observation = observations[year];
        if (observation.populationByAgeGroup.size() != groupCount ||
            (year > 0 &&
             (observations[year - 1].year == std::numeric_limits<int>::max() ||
              observation.year != observations[year - 1].year + 1))) {
            throw std::invalid_argument(
                "age-structured census years must be consecutive with matching age groups");
        }
        double total = 0.0;
        for (float population : observation.populationByAgeGroup) {
            if (!std::isfinite(population) || population < 0.0f) {
                throw std::invalid_argument(
                    "census populations must be finite and nonnegative");
            }
            total += population;
        }
        if (total > carryingCapacity) {
            throw std::invalid_argument(
                "census population exceeds carrying capacity");
        }
    }

    std::vector<double> exposures(groupCount, 0.0);
    std::vector<double> transitions(groupCount, 0.0);
    for (std::size_t year = 0; year + 1 < observations.size(); ++year) {
        const auto& current = observations[year].populationByAgeGroup;
        const auto& next = observations[year + 1].populationByAgeGroup;
        for (std::size_t group = 0; group + 2 < groupCount; ++group) {
            exposures[group] += static_cast<double>(current[group]) * current[group];
            transitions[group] +=
                static_cast<double>(current[group]) * next[group + 1];
        }
    }

    AgeStructuredPopulationConfig config;
    config.initialPopulationByAgeGroup = observations.front().populationByAgeGroup;
    config.carryingCapacity = carryingCapacity;
    config.annualSurvivalRateByAgeGroup.assign(groupCount, 0.0f);
    config.annualBirthsPerPersonByAgeGroup.assign(groupCount, 0.0f);
    for (std::size_t group = 0; group + 2 < groupCount; ++group) {
        if (exposures[group] <= 0.0) {
            throw std::invalid_argument(
                "census series has no exposure to estimate an age-group survival rate");
        }
        config.annualSurvivalRateByAgeGroup[group] = static_cast<float>(
            std::clamp(transitions[group] / exposures[group], 0.0, 1.0));
    }

    const std::size_t penultimateGroup = groupCount - 2;
    const std::size_t openGroup = groupCount - 1;
    double penultimateSquares = 0.0;
    double openSquares = 0.0;
    double crossProduct = 0.0;
    double penultimateTransition = 0.0;
    double openTransition = 0.0;
    double fertilityExposure = 0.0;
    double fertilityBirths = 0.0;
    for (std::size_t year = 0; year + 1 < observations.size(); ++year) {
        const auto& current = observations[year].populationByAgeGroup;
        const auto& next = observations[year + 1].populationByAgeGroup;
        const double penultimate = current[penultimateGroup];
        const double open = current[openGroup];
        const double nextOpen = next[openGroup];
        penultimateSquares += penultimate * penultimate;
        openSquares += open * open;
        crossProduct += penultimate * open;
        penultimateTransition += penultimate * nextOpen;
        openTransition += open * nextOpen;
        double weightedPopulation = 0.0;
        for (std::size_t group = 0; group < groupCount; ++group) {
            weightedPopulation += static_cast<double>(current[group]) *
                                  fertilityWeights[group];
        }
        fertilityExposure += weightedPopulation * weightedPopulation;
        fertilityBirths += weightedPopulation * next.front();
    }
    const double determinant =
        penultimateSquares * openSquares - crossProduct * crossProduct;
    if (penultimateSquares <= 0.0 || openSquares <= 0.0 ||
        determinant <= penultimateSquares * openSquares * 1e-10 ||
        fertilityExposure <= 0.0) {
        throw std::invalid_argument(
            "census series lacks independent open-age or fertility exposure");
    }
    const double unconstrainedPenultimate =
        (penultimateTransition * openSquares -
         openTransition * crossProduct) / determinant;
    const double unconstrainedOpen =
        (openTransition * penultimateSquares -
         penultimateTransition * crossProduct) / determinant;
    std::vector<std::pair<double, double>> survivalCandidates;
    if (unconstrainedPenultimate >= 0.0 && unconstrainedPenultimate <= 1.0 &&
        unconstrainedOpen >= 0.0 && unconstrainedOpen <= 1.0) {
        survivalCandidates.emplace_back(unconstrainedPenultimate,
                                        unconstrainedOpen);
    }
    survivalCandidates.emplace_back(
        0.0, std::clamp(openTransition / openSquares, 0.0, 1.0));
    survivalCandidates.emplace_back(
        1.0, std::clamp((openTransition - crossProduct) / openSquares,
                        0.0, 1.0));
    survivalCandidates.emplace_back(
        std::clamp(penultimateTransition / penultimateSquares, 0.0, 1.0),
        0.0);
    survivalCandidates.emplace_back(
        std::clamp((penultimateTransition - crossProduct) /
                       penultimateSquares,
                   0.0, 1.0),
        1.0);
    auto survivalError = [&](const auto& candidate) {
        double error = 0.0;
        for (std::size_t year = 0; year + 1 < observations.size(); ++year) {
            const auto& current = observations[year].populationByAgeGroup;
            const auto& next = observations[year + 1].populationByAgeGroup;
            const double residual = next[openGroup] -
                current[penultimateGroup] * candidate.first -
                current[openGroup] * candidate.second;
            error += residual * residual;
        }
        return error;
    };
    const auto bestSurvival = *std::min_element(
        survivalCandidates.begin(), survivalCandidates.end(),
        [&](const auto& lhs, const auto& rhs) {
            return survivalError(lhs) < survivalError(rhs);
        });
    config.annualSurvivalRateByAgeGroup[penultimateGroup] =
        static_cast<float>(bestSurvival.first);
    config.annualSurvivalRateByAgeGroup[openGroup] =
        static_cast<float>(bestSurvival.second);
    const double fertilityScale =
        std::max(0.0, fertilityBirths / fertilityExposure);
    if (!std::isfinite(fertilityScale) ||
        fertilityScale > std::numeric_limits<float>::max()) {
        throw std::overflow_error("fitted fertility scale exceeds float range");
    }
    for (std::size_t group = 0; group < groupCount; ++group) {
        const double rate = fertilityScale * fertilityWeights[group];
        if (!std::isfinite(rate) || rate > std::numeric_limits<float>::max()) {
            throw std::overflow_error("fitted birth rate exceeds float range");
        }
        config.annualBirthsPerPersonByAgeGroup[group] =
            static_cast<float>(rate);
    }

    for (std::size_t year = 0; year + 1 < observations.size(); ++year) {
        const auto& current = observations[year].populationByAgeGroup;
        double projectedSurvivors = 0.0;
        double weightedPopulation = 0.0;
        for (std::size_t group = 0; group < groupCount; ++group) {
            projectedSurvivors += current[group] *
                config.annualSurvivalRateByAgeGroup[group];
            weightedPopulation += static_cast<double>(current[group]) *
                                  fertilityWeights[group];
        }
        const double projectedBirths = weightedPopulation * fertilityScale;
        if (projectedSurvivors + projectedBirths >
            static_cast<double>(carryingCapacity) + 1e-4) {
            throw std::invalid_argument(
                "fitted cohort model exceeds carrying capacity; use a larger capacity or review census assumptions");
        }
    }
    return config;
}

RegionalAgeStructuredPopulationRates
CalibrateRegionalAgeStructuredPopulationRates(
    const std::vector<RegionalAgeStructuredCensusObservation>& observations,
    const std::vector<RegionalAgeStructuredMigrationObservation>& migration,
    const std::vector<float>& fertilityWeights) {
    if (observations.size() < 2 ||
        migration.size() + 1 != observations.size() ||
        fertilityWeights.size() < 2) {
        throw std::invalid_argument(
            "regional cohort calibration requires census intervals, "
            "migration data, and at least two age groups");
    }
    const std::size_t regionCount =
        observations.front().populationByRegionAndAgeGroup.size();
    const std::size_t groupCount = fertilityWeights.size();
    if (regionCount == 0) {
        throw std::invalid_argument(
            "regional cohort calibration requires at least one region");
    }
    bool hasFertileWeight = false;
    for (float weight : fertilityWeights) {
        if (!std::isfinite(weight) || weight < 0.0f) {
            throw std::invalid_argument(
                "fertility weights must be finite and nonnegative");
        }
        hasFertileWeight = hasFertileWeight || weight > 0.0f;
    }
    if (!hasFertileWeight) {
        throw std::invalid_argument("at least one fertility weight must be positive");
    }

    for (std::size_t year = 0; year < observations.size(); ++year) {
        const auto& observation = observations[year];
        if (observation.populationByRegionAndAgeGroup.size() != regionCount ||
            (year > 0 &&
             (observations[year - 1].year == std::numeric_limits<int>::max() ||
              observation.year != observations[year - 1].year + 1))) {
            throw std::invalid_argument(
                "regional census years must be consecutive with consistent regions");
        }
        for (const auto& groups : observation.populationByRegionAndAgeGroup) {
            if (groups.size() != groupCount) {
                throw std::invalid_argument(
                    "regional census rows must match the fertility age groups");
            }
            for (float population : groups) {
                if (!std::isfinite(population) || population < 0.0f) {
                    throw std::invalid_argument(
                        "regional census populations must be finite and nonnegative");
                }
            }
        }
    }

    std::vector<std::vector<std::vector<double>>> adjustedNextByInterval(
        migration.size(),
        std::vector<std::vector<double>>(regionCount,
                                         std::vector<double>(groupCount)));
    for (std::size_t interval = 0; interval < migration.size(); ++interval) {
        const auto& flow = migration[interval];
        if (flow.fromYear != observations[interval].year ||
            flow.netMigrationByRegionAndAgeGroup.size() != regionCount) {
            throw std::invalid_argument(
                "regional migration intervals must match census years and regions");
        }
        std::vector<double> netByAge(groupCount, 0.0);
        std::vector<double> magnitudeByAge(groupCount, 0.0);
        for (std::size_t region = 0; region < regionCount; ++region) {
            const auto& net = flow.netMigrationByRegionAndAgeGroup[region];
            if (net.size() != groupCount) {
                throw std::invalid_argument(
                    "regional migration rows must match the census age groups");
            }
            for (std::size_t group = 0; group < groupCount; ++group) {
                if (!std::isfinite(net[group])) {
                    throw std::invalid_argument(
                        "regional migration values must be finite");
                }
                netByAge[group] += net[group];
                magnitudeByAge[group] += std::abs(net[group]);
                const double adjusted =
                    observations[interval + 1]
                        .populationByRegionAndAgeGroup[region][group] -
                    net[group];
                if (adjusted < -1e-5) {
                    throw std::invalid_argument(
                        "net migration exceeds the observed destination cohort");
                }
                adjustedNextByInterval[interval][region][group] =
                    std::max(0.0, adjusted);
            }
        }
        for (std::size_t group = 0; group < groupCount; ++group) {
            if (std::abs(netByAge[group]) >
                1e-5 * std::max(1.0, magnitudeByAge[group])) {
                throw std::invalid_argument(
                    "regional net migration must conserve each age group");
            }
        }
    }

    std::vector<double> exposures(groupCount, 0.0);
    std::vector<double> transitions(groupCount, 0.0);
    double penultimateSquares = 0.0;
    double openSquares = 0.0;
    double crossProduct = 0.0;
    double penultimateTransition = 0.0;
    double openTransition = 0.0;
    double fertilityExposure = 0.0;
    double fertilityBirths = 0.0;
    const std::size_t penultimateGroup = groupCount - 2;
    const std::size_t openGroup = groupCount - 1;
    for (std::size_t interval = 0; interval < migration.size(); ++interval) {
        for (std::size_t region = 0; region < regionCount; ++region) {
            const auto& current =
                observations[interval].populationByRegionAndAgeGroup[region];
            const auto& adjustedNext = adjustedNextByInterval[interval][region];
            for (std::size_t group = 0; group + 2 < groupCount; ++group) {
                exposures[group] +=
                    static_cast<double>(current[group]) * current[group];
                transitions[group] +=
                    static_cast<double>(current[group]) * adjustedNext[group + 1];
            }

            const double penultimate = current[penultimateGroup];
            const double open = current[openGroup];
            penultimateSquares += penultimate * penultimate;
            openSquares += open * open;
            crossProduct += penultimate * open;
            penultimateTransition +=
                penultimate * adjustedNext[openGroup];
            openTransition += open * adjustedNext[openGroup];

            double weightedPopulation = 0.0;
            for (std::size_t group = 0; group < groupCount; ++group) {
                weightedPopulation += static_cast<double>(current[group]) *
                                      fertilityWeights[group];
            }
            fertilityExposure += weightedPopulation * weightedPopulation;
            fertilityBirths += weightedPopulation * adjustedNext.front();
        }
    }

    RegionalAgeStructuredPopulationRates rates;
    rates.annualSurvivalRateByAgeGroup.assign(groupCount, 0.0f);
    rates.annualBirthsPerPersonByAgeGroup.assign(groupCount, 0.0f);
    for (std::size_t group = 0; group + 2 < groupCount; ++group) {
        if (exposures[group] <= 0.0) {
            throw std::invalid_argument(
                "regional census has no exposure to estimate an age-group survival rate");
        }
        rates.annualSurvivalRateByAgeGroup[group] = static_cast<float>(
            std::clamp(transitions[group] / exposures[group], 0.0, 1.0));
    }

    const double determinant =
        penultimateSquares * openSquares - crossProduct * crossProduct;
    if (penultimateSquares <= 0.0 || openSquares <= 0.0 ||
        determinant <= penultimateSquares * openSquares * 1e-10 ||
        fertilityExposure <= 0.0) {
        throw std::invalid_argument(
            "regional census lacks independent open-age or fertility exposure");
    }
    const double unconstrainedPenultimate =
        (penultimateTransition * openSquares -
         openTransition * crossProduct) / determinant;
    const double unconstrainedOpen =
        (openTransition * penultimateSquares -
         penultimateTransition * crossProduct) / determinant;
    std::vector<std::pair<double, double>> survivalCandidates;
    if (unconstrainedPenultimate >= 0.0 && unconstrainedPenultimate <= 1.0 &&
        unconstrainedOpen >= 0.0 && unconstrainedOpen <= 1.0) {
        survivalCandidates.emplace_back(unconstrainedPenultimate,
                                        unconstrainedOpen);
    }
    survivalCandidates.emplace_back(
        0.0, std::clamp(openTransition / openSquares, 0.0, 1.0));
    survivalCandidates.emplace_back(
        1.0, std::clamp((openTransition - crossProduct) / openSquares,
                        0.0, 1.0));
    survivalCandidates.emplace_back(
        std::clamp(penultimateTransition / penultimateSquares, 0.0, 1.0),
        0.0);
    survivalCandidates.emplace_back(
        std::clamp((penultimateTransition - crossProduct) /
                       penultimateSquares,
                   0.0, 1.0),
        1.0);
    auto survivalError = [&](const auto& candidate) {
        double error = 0.0;
        for (std::size_t interval = 0; interval < migration.size(); ++interval) {
            for (std::size_t region = 0; region < regionCount; ++region) {
                const auto& current =
                    observations[interval].populationByRegionAndAgeGroup[region];
                const double residual = adjustedNextByInterval[interval][region]
                                            [openGroup] -
                    current[penultimateGroup] * candidate.first -
                    current[openGroup] * candidate.second;
                error += residual * residual;
            }
        }
        return error;
    };
    const auto bestSurvival = *std::min_element(
        survivalCandidates.begin(), survivalCandidates.end(),
        [&](const auto& lhs, const auto& rhs) {
            return survivalError(lhs) < survivalError(rhs);
        });
    rates.annualSurvivalRateByAgeGroup[penultimateGroup] =
        static_cast<float>(bestSurvival.first);
    rates.annualSurvivalRateByAgeGroup[openGroup] =
        static_cast<float>(bestSurvival.second);

    const double fertilityScale =
        std::max(0.0, fertilityBirths / fertilityExposure);
    if (!std::isfinite(fertilityScale) ||
        fertilityScale > std::numeric_limits<float>::max()) {
        throw std::overflow_error(
            "fitted regional fertility scale exceeds float range");
    }
    for (std::size_t group = 0; group < groupCount; ++group) {
        const double rate = fertilityScale * fertilityWeights[group];
        if (!std::isfinite(rate) || rate > std::numeric_limits<float>::max()) {
            throw std::overflow_error(
                "fitted regional birth rate exceeds float range");
        }
        rates.annualBirthsPerPersonByAgeGroup[group] =
            static_cast<float>(rate);
    }
    return rates;
}

std::vector<RegionalAgeStructuredMigrationObservation>
InferRegionalAgeStructuredNetMigration(
    const std::vector<RegionalAgeStructuredCensusObservation>& observations,
    const RegionalAgeStructuredPopulationRates& rates) {
    const std::size_t groupCount = rates.annualSurvivalRateByAgeGroup.size();
    if (observations.size() < 2 || groupCount < 2 ||
        rates.annualBirthsPerPersonByAgeGroup.size() != groupCount) {
        throw std::invalid_argument(
            "regional migration inference requires census intervals and matching rates");
    }
    for (std::size_t group = 0; group < groupCount; ++group) {
        const float survival = rates.annualSurvivalRateByAgeGroup[group];
        const float births = rates.annualBirthsPerPersonByAgeGroup[group];
        if (!std::isfinite(survival) || survival < 0.0f || survival > 1.0f ||
            !std::isfinite(births) || births < 0.0f) {
            throw std::invalid_argument(
                "regional demographic rates must be finite and in range");
        }
    }

    const std::size_t regionCount =
        observations.front().populationByRegionAndAgeGroup.size();
    if (regionCount == 0) {
        throw std::invalid_argument(
            "regional migration inference requires at least one region");
    }
    for (std::size_t year = 0; year < observations.size(); ++year) {
        const auto& observation = observations[year];
        if (observation.populationByRegionAndAgeGroup.size() != regionCount ||
            (year > 0 &&
             (observations[year - 1].year == std::numeric_limits<int>::max() ||
              observation.year != observations[year - 1].year + 1))) {
            throw std::invalid_argument(
                "regional census years must be consecutive with consistent regions");
        }
        for (const auto& groups : observation.populationByRegionAndAgeGroup) {
            if (groups.size() != groupCount) {
                throw std::invalid_argument(
                    "regional census rows must match the demographic rates");
            }
            for (float population : groups) {
                if (!std::isfinite(population) || population < 0.0f) {
                    throw std::invalid_argument(
                        "regional census populations must be finite and nonnegative");
                }
            }
        }
    }

    const std::size_t openGroup = groupCount - 1;
    std::vector<RegionalAgeStructuredMigrationObservation> inferred;
    inferred.reserve(observations.size() - 1);
    for (std::size_t interval = 0; interval + 1 < observations.size();
         ++interval) {
        const auto& current =
            observations[interval].populationByRegionAndAgeGroup;
        const auto& next =
            observations[interval + 1].populationByRegionAndAgeGroup;
        std::vector<std::vector<float>> netByRegion(
            regionCount, std::vector<float>(groupCount, 0.0f));
        std::vector<double> netByGroup(groupCount, 0.0);
        std::vector<double> scaleByGroup(groupCount, 0.0);

        for (std::size_t region = 0; region < regionCount; ++region) {
            std::vector<double> demographicNext(groupCount, 0.0);
            for (std::size_t group = 0; group < groupCount; ++group) {
                demographicNext[0] +=
                    static_cast<double>(current[region][group]) *
                    rates.annualBirthsPerPersonByAgeGroup[group];
                if (group > 0) {
                    demographicNext[group] +=
                        static_cast<double>(current[region][group - 1]) *
                        rates.annualSurvivalRateByAgeGroup[group - 1];
                }
            }
            demographicNext[openGroup] +=
                static_cast<double>(current[region][openGroup]) *
                rates.annualSurvivalRateByAgeGroup[openGroup];

            for (std::size_t group = 0; group < groupCount; ++group) {
                const double net =
                    static_cast<double>(next[region][group]) -
                    demographicNext[group];
                if (!std::isfinite(net) ||
                    std::abs(net) > std::numeric_limits<float>::max()) {
                    throw std::overflow_error(
                        "inferred regional net migration exceeds float range");
                }
                netByRegion[region][group] = static_cast<float>(net);
                netByGroup[group] += net;
                scaleByGroup[group] +=
                    std::abs(static_cast<double>(next[region][group])) +
                    std::abs(demographicNext[group]);
            }
        }

        for (std::size_t group = 0; group < groupCount; ++group) {
            if (std::abs(netByGroup[group]) >
                1e-4 * std::max(1.0, scaleByGroup[group])) {
                throw std::invalid_argument(
                    "census deltas are inconsistent with shared rates and "
                    "conservative regional migration");
            }

            // Remove the small aggregate residual accepted above (for example,
            // census rounding) so the returned float observations are exactly
            // conservative when consumed by the rate calibration API.
            double roundedNetSum = 0.0;
            for (std::size_t region = 0; region + 1 < regionCount; ++region) {
                roundedNetSum += netByRegion[region][group];
            }
            if (std::abs(roundedNetSum) > std::numeric_limits<float>::max()) {
                throw std::overflow_error(
                    "conservative net migration exceeds float range");
            }
            netByRegion.back()[group] =
                static_cast<float>(-roundedNetSum);
        }
        inferred.push_back(
            {observations[interval].year, std::move(netByRegion)});
    }
    return inferred;
}

} // namespace Population
} // namespace Civilization
} // namespace StonePotatoes
