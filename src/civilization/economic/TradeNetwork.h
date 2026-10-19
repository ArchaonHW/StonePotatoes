// SPDX-License-Identifier: MIT
// Copyright (c) 2026 ArchaonHW
#pragma once

#include "civilization/economic/SystemDynamics.h"

#include <cstddef>
#include <string>
#include <vector>

namespace StonePotatoes {
namespace Civilization {
namespace Economic {

/** Configuration for a monthly stock-and-trade network for one commodity. */
struct TradeNetworkConfig {
    std::vector<float> initialStockByRegion;
    std::vector<float> storageCapacityByRegion;
    std::vector<float> monthlyProductionByRegion;
    std::vector<float> monthlyConsumptionByRegion;
    // Absolute commodity units per month; rows are source regions.
    std::vector<std::vector<float>> monthlyTradeFromRegionToRegion;
};

/**
 * Build an inventory model with monthly production/consumption and directed
 * transfers. Consumption is bounded by local stock plus production; accepted
 * transfers conserve stock and respect source inventory and destination space.
 */
SDModel BuildTradeNetworkModel(const TradeNetworkConfig& config);

std::string TradeStockName(std::size_t regionIndex);
std::string TradeProductionParameterName(std::size_t regionIndex);
std::string TradeConsumptionParameterName(std::size_t regionIndex);
std::string TradeRouteParameterName(std::size_t fromRegionIndex,
                                    std::size_t toRegionIndex);

} // namespace Economic
} // namespace Civilization
} // namespace StonePotatoes
