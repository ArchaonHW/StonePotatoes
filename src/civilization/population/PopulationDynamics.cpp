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
} // namespace

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

} // namespace Population
} // namespace Civilization
} // namespace StonePotatoes
