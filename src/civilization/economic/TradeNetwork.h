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
    // Optional treasury for transport costs. A cost matrix enables costs;
    // rows are sources and values are funds charged per unit shipped.
    std::vector<float> initialFundsByRegion;
    std::vector<float> monthlyFundsIncomeByRegion;
    std::vector<std::vector<float>>
        monthlyTransportCostPerUnitFromRegionToRegion;
};

/**
 * Build an inventory model with monthly production/consumption and directed
 * transfers. Consumption is bounded by local stock plus production; accepted
 * transfers respect source inventory and destination space. When a transport
 * cost matrix is configured, shipments are also bounded by source funds.
 */
SDModel BuildTradeNetworkModel(const TradeNetworkConfig& config);

std::string TradeStockName(std::size_t regionIndex);
std::string TradeProductionParameterName(std::size_t regionIndex);
std::string TradeConsumptionParameterName(std::size_t regionIndex);
std::string TradeRouteParameterName(std::size_t fromRegionIndex,
                                    std::size_t toRegionIndex);
std::string TradeFundsStockName(std::size_t regionIndex);
std::string TradeFundsIncomeParameterName(std::size_t regionIndex);
std::string TradeRouteTransportCostParameterName(
    std::size_t fromRegionIndex, std::size_t toRegionIndex);

} // namespace Economic
} // namespace Civilization
} // namespace StonePotatoes
