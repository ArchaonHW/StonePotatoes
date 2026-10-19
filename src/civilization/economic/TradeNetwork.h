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
    // Optional treasury for route costs and purchases.
    std::vector<float> initialFundsByRegion;
    std::vector<float> monthlyFundsIncomeByRegion;
    // Optional cost per commodity unit; rows are sources, columns destinations.
    std::vector<std::vector<float>>
        monthlyTransportCostPerUnitFromRegionToRegion;
    // Optional transaction price per commodity unit; buyers pay sellers.
    std::vector<std::vector<float>>
        monthlyTransactionPricePerUnitFromRegionToRegion;
};

/**
 * Build an inventory model with monthly production/consumption and directed
 * transfers. Consumption is bounded by local stock plus production; accepted
 * transfers respect source inventory and destination space. Optional route
 * prices and transport costs are settled against regional funds balances.
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
std::string TradeRouteTransactionPriceParameterName(
    std::size_t fromRegionIndex, std::size_t toRegionIndex);

} // namespace Economic
} // namespace Civilization
} // namespace StonePotatoes
