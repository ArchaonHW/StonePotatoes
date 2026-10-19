// SPDX-License-Identifier: MIT
// Copyright (c) 2026 ArchaonHW
#pragma once

#include "civilization/economic/SystemDynamics.h"

namespace StonePotatoes {
namespace Civilization {
namespace Population {

/** Configuration for a single-region logistic population stock. */
struct PopulationConfig {
    float initialPopulation = 1000.0f;
    float carryingCapacity = 10000.0f;
    // Intrinsic growth per month; positive grows and negative declines.
    float intrinsicGrowthRatePerMonth = 0.01f;
};

/**
 * Build a reusable stock-flow model with one `population` stock.
 * The net flow is r * P * (1 - P / K), bounded to [0, K].
 * The returned model uses monthly steps by default and exposes its rate as the
 * `intrinsic_growth_rate_per_month` parameter for scenario changes.
 */
Economic::SDModel BuildPopulationModel(const PopulationConfig& config);

} // namespace Population
} // namespace Civilization
} // namespace StonePotatoes
