// SPDX-License-Identifier: MIT
// Copyright (c) 2026 ArchaonHW
#include "civilization/population/PopulationCalibration.h"

#include <cmath>
#include <limits>
#include <stdexcept>
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

} // namespace Population
} // namespace Civilization
} // namespace StonePotatoes
