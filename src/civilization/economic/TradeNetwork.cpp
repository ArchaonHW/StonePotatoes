// SPDX-License-Identifier: MIT
// Copyright (c) 2026 ArchaonHW
#include "TradeNetwork.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace StonePotatoes {
namespace Civilization {
namespace Economic {
namespace {

std::string IndexedName(const char* prefix, std::size_t index) {
    return std::string(prefix) + std::to_string(index);
}

struct TradeStepProjection {
    std::vector<float> stockAfterProductionAndConsumption;
    std::vector<float> tradeDelta;
    std::vector<float> fundsAfterTrade;
};

TradeStepProjection ProjectTradeStep(
    const SDState& state,
    const std::vector<std::string>& stockNames,
    const std::vector<std::string>& productionParameterNames,
    const std::vector<std::string>& consumptionParameterNames,
    const std::vector<float>& capacities,
    const std::vector<std::vector<std::string>>& routeParameterNames,
    const std::vector<std::string>& fundsStockNames,
    const std::vector<std::string>& fundsIncomeParameterNames,
    const std::vector<std::vector<std::string>>&
        routeTransportCostParameterNames) {
    const std::size_t regionCount = stockNames.size();
    TradeStepProjection result{
        std::vector<float>(regionCount, 0.0f),
        std::vector<float>(regionCount, 0.0f),
        fundsStockNames.empty() ? std::vector<float>{}
                                : std::vector<float>(regionCount, 0.0f)};
    std::vector<std::vector<float>> requestedTransfers(
        regionCount, std::vector<float>(regionCount, 0.0f));

    for (std::size_t region = 0; region < regionCount; ++region) {
        const float stock = state.S(stockNames[region]);
        const float rawProduction = state.P(productionParameterNames[region]);
        const float rawConsumption =
            state.P(consumptionParameterNames[region]);
        const float production = std::isfinite(rawProduction)
            ? std::max(0.0f, rawProduction) : 0.0f;
        const float consumption = std::isfinite(rawConsumption)
            ? std::max(0.0f, rawConsumption) : 0.0f;
        const float available = stock + production;
        const float consumed = std::min(consumption, available);
        result.stockAfterProductionAndConsumption[region] =
            std::clamp(available - consumed, 0.0f, capacities[region]);
    }

    for (std::size_t source = 0; source < regionCount; ++source) {
        float requestedOutflow = 0.0f;
        for (std::size_t destination = 0; destination < regionCount;
             ++destination) {
            const float rawRequest =
                state.P(routeParameterNames[source][destination]);
            const float request = source != destination &&
                    std::isfinite(rawRequest)
                ? std::max(0.0f, rawRequest) : 0.0f;
            requestedTransfers[source][destination] = request;
            requestedOutflow += request;
        }
        const float sourceStock =
            result.stockAfterProductionAndConsumption[source];
        if (requestedOutflow > sourceStock && requestedOutflow > 0.0f) {
            const float sourceScale = sourceStock / requestedOutflow;
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
        const float room = std::max(
            0.0f, capacities[destination] -
                      result.stockAfterProductionAndConsumption[destination]);
        if (requestedInflow > room && requestedInflow > 0.0f) {
            destinationScales[destination] = room / requestedInflow;
        }
    }

    for (std::size_t source = 0; source < regionCount; ++source) {
        float fundsScale = 1.0f;
        float availableFunds = 0.0f;
        if (!fundsStockNames.empty()) {
            const float rawIncome =
                state.P(fundsIncomeParameterNames[source]);
            const float income = std::isfinite(rawIncome)
                ? std::max(0.0f, rawIncome) : 0.0f;
            availableFunds = std::max(
                0.0f, state.S(fundsStockNames[source])) + income;

            float requestedTransportCost = 0.0f;
            for (std::size_t destination = 0; destination < regionCount;
                 ++destination) {
                const float rawCost = state.P(
                    routeTransportCostParameterNames[source][destination]);
                const float unitCost = std::isfinite(rawCost)
                    ? std::max(0.0f, rawCost) : 0.0f;
                requestedTransportCost +=
                    requestedTransfers[source][destination] *
                    destinationScales[destination] * unitCost;
            }
            if (requestedTransportCost > availableFunds &&
                requestedTransportCost > 0.0f) {
                fundsScale = availableFunds / requestedTransportCost;
            }
        }

        float acceptedTransportCost = 0.0f;
        for (std::size_t destination = 0; destination < regionCount;
             ++destination) {
            float transfer = requestedTransfers[source][destination] *
                destinationScales[destination] * fundsScale;
            if (!fundsStockNames.empty()) {
                const float rawCost = state.P(
                    routeTransportCostParameterNames[source][destination]);
                const float unitCost = std::isfinite(rawCost)
                    ? std::max(0.0f, rawCost) : 0.0f;
                acceptedTransportCost += transfer * unitCost;
            }
            result.tradeDelta[source] -= transfer;
            result.tradeDelta[destination] += transfer;
        }
        if (!fundsStockNames.empty()) {
            result.fundsAfterTrade[source] = std::max(
                0.0f, availableFunds - acceptedTransportCost);
        }
    }
    return result;
}

} // namespace

std::string TradeStockName(std::size_t regionIndex) {
    return IndexedName("trade_stock_region_", regionIndex);
}

std::string TradeProductionParameterName(std::size_t regionIndex) {
    return IndexedName("trade_production_per_month_region_", regionIndex);
}

std::string TradeConsumptionParameterName(std::size_t regionIndex) {
    return IndexedName("trade_consumption_per_month_region_", regionIndex);
}

std::string TradeRouteParameterName(std::size_t fromRegionIndex,
                                    std::size_t toRegionIndex) {
    return IndexedName("trade_route_per_month_region_", fromRegionIndex) +
           "_to_" + std::to_string(toRegionIndex);
}

std::string TradeFundsStockName(std::size_t regionIndex) {
    return IndexedName("trade_funds_region_", regionIndex);
}

std::string TradeFundsIncomeParameterName(std::size_t regionIndex) {
    return IndexedName("trade_funds_income_per_month_region_", regionIndex);
}

std::string TradeRouteTransportCostParameterName(
    std::size_t fromRegionIndex, std::size_t toRegionIndex) {
    return IndexedName("trade_transport_cost_per_unit_region_",
                       fromRegionIndex) + "_to_" +
           std::to_string(toRegionIndex);
}

SDModel BuildTradeNetworkModel(const TradeNetworkConfig& config) {
    const std::size_t regionCount = config.initialStockByRegion.size();
    if (regionCount == 0 ||
        config.storageCapacityByRegion.size() != regionCount ||
        config.monthlyProductionByRegion.size() != regionCount ||
        config.monthlyConsumptionByRegion.size() != regionCount ||
        config.monthlyTradeFromRegionToRegion.size() != regionCount) {
        throw std::invalid_argument(
            "trade network vectors must have matching nonzero sizes");
    }

    const bool hasTransportCosts =
        !config.monthlyTransportCostPerUnitFromRegionToRegion.empty();
    if ((!hasTransportCosts && !config.initialFundsByRegion.empty()) ||
        (!hasTransportCosts && !config.monthlyFundsIncomeByRegion.empty()) ||
        (hasTransportCosts &&
         config.initialFundsByRegion.size() != regionCount) ||
        (!config.monthlyFundsIncomeByRegion.empty() &&
         config.monthlyFundsIncomeByRegion.size() != regionCount) ||
        (hasTransportCosts &&
         config.monthlyTransportCostPerUnitFromRegionToRegion.size() !=
             regionCount)) {
        throw std::invalid_argument(
            "trade funds and transport cost vectors must match regions");
    }

    for (std::size_t region = 0; region < regionCount; ++region) {
        if (hasTransportCosts) {
            const float funds = config.initialFundsByRegion[region];
            const float income = config.monthlyFundsIncomeByRegion.empty()
                ? 0.0f : config.monthlyFundsIncomeByRegion[region];
            if (!std::isfinite(funds) || funds < 0.0f ||
                !std::isfinite(income) || income < 0.0f ||
                config.monthlyTransportCostPerUnitFromRegionToRegion[region]
                        .size() != regionCount) {
                throw std::invalid_argument(
                    "invalid trade funds or transport costs");
            }
            for (std::size_t destination = 0; destination < regionCount;
                 ++destination) {
                const float cost =
                    config.monthlyTransportCostPerUnitFromRegionToRegion
                        [region][destination];
                if (!std::isfinite(cost) || cost < 0.0f ||
                    (region == destination && cost != 0.0f)) {
                    throw std::invalid_argument(
                        "transport costs must be finite, nonnegative, and "
                        "exclude self-routes");
                }
            }
        }
    }

    for (std::size_t region = 0; region < regionCount; ++region) {
        const float initial = config.initialStockByRegion[region];
        const float capacity = config.storageCapacityByRegion[region];
        const float production = config.monthlyProductionByRegion[region];
        const float consumption = config.monthlyConsumptionByRegion[region];
        if (!std::isfinite(initial) || !std::isfinite(capacity) ||
            !std::isfinite(production) || !std::isfinite(consumption) ||
            capacity <= 0.0f || initial < 0.0f || initial > capacity ||
            production < 0.0f || consumption < 0.0f ||
            config.monthlyTradeFromRegionToRegion[region].size() !=
                regionCount) {
            throw std::invalid_argument("invalid trade network configuration");
        }
        for (std::size_t destination = 0; destination < regionCount;
             ++destination) {
            const float trade =
                config.monthlyTradeFromRegionToRegion[region][destination];
            if (!std::isfinite(trade) || trade < 0.0f ||
                (region == destination && trade != 0.0f)) {
                throw std::invalid_argument(
                    "trade routes must be finite, nonnegative, and exclude "
                    "self-transfers");
            }
        }
    }

    std::vector<std::string> stockNames;
    std::vector<std::string> productionParameterNames;
    std::vector<std::string> consumptionParameterNames;
    std::vector<std::vector<std::string>> routeParameterNames(
        regionCount, std::vector<std::string>(regionCount));
    std::vector<std::string> fundsStockNames;
    std::vector<std::string> fundsIncomeParameterNames;
    std::vector<std::vector<std::string>> routeTransportCostParameterNames;
    if (hasTransportCosts) {
        routeTransportCostParameterNames.assign(
            regionCount, std::vector<std::string>(regionCount));
        fundsStockNames.reserve(regionCount);
        fundsIncomeParameterNames.reserve(regionCount);
    }
    stockNames.reserve(regionCount);
    productionParameterNames.reserve(regionCount);
    consumptionParameterNames.reserve(regionCount);

    SDModel model;
    for (std::size_t region = 0; region < regionCount; ++region) {
        const std::string stock = TradeStockName(region);
        const std::string production = TradeProductionParameterName(region);
        const std::string consumption = TradeConsumptionParameterName(region);
        stockNames.push_back(stock);
        productionParameterNames.push_back(production);
        consumptionParameterNames.push_back(consumption);
        model.AddStock(stock, config.initialStockByRegion[region], 0.0f,
                       config.storageCapacityByRegion[region]);
        model.SetParam(production, config.monthlyProductionByRegion[region]);
        model.SetParam(consumption, config.monthlyConsumptionByRegion[region]);
        if (hasTransportCosts) {
            const std::string fundsStock = TradeFundsStockName(region);
            const std::string fundsIncome =
                TradeFundsIncomeParameterName(region);
            fundsStockNames.push_back(fundsStock);
            fundsIncomeParameterNames.push_back(fundsIncome);
            model.AddStock(fundsStock, config.initialFundsByRegion[region]);
            model.SetParam(fundsIncome,
                config.monthlyFundsIncomeByRegion.empty()
                    ? 0.0f : config.monthlyFundsIncomeByRegion[region]);
        }
        for (std::size_t destination = 0; destination < regionCount;
             ++destination) {
            const std::string route =
                TradeRouteParameterName(region, destination);
            routeParameterNames[region][destination] = route;
            model.SetParam(
                route,
                config.monthlyTradeFromRegionToRegion[region][destination]);
            if (hasTransportCosts) {
                const std::string cost =
                    TradeRouteTransportCostParameterName(region, destination);
                routeTransportCostParameterNames[region][destination] = cost;
                model.SetParam(cost,
                    config.monthlyTransportCostPerUnitFromRegionToRegion
                        [region][destination]);
            }
        }
    }

    for (std::size_t region = 0; region < regionCount; ++region) {
        model.AddFlow(stockNames[region], [region, stockNames,
            productionParameterNames, consumptionParameterNames,
            capacities = config.storageCapacityByRegion,
            routeParameterNames, fundsStockNames,
            fundsIncomeParameterNames,
            routeTransportCostParameterNames](const SDState& state) {
            const TradeStepProjection projection = ProjectTradeStep(
                state, stockNames, productionParameterNames,
                consumptionParameterNames, capacities, routeParameterNames,
                fundsStockNames, fundsIncomeParameterNames,
                routeTransportCostParameterNames);
            return projection.stockAfterProductionAndConsumption[region] -
                       state.S(stockNames[region]) +
                   projection.tradeDelta[region];
        });
    }

    for (std::size_t region = 0; region < fundsStockNames.size(); ++region) {
        model.AddFlow(fundsStockNames[region], [region, stockNames,
            productionParameterNames, consumptionParameterNames,
            capacities = config.storageCapacityByRegion,
            routeParameterNames, fundsStockNames,
            fundsIncomeParameterNames,
            routeTransportCostParameterNames](const SDState& state) {
            const TradeStepProjection projection = ProjectTradeStep(
                state, stockNames, productionParameterNames,
                consumptionParameterNames, capacities, routeParameterNames,
                fundsStockNames, fundsIncomeParameterNames,
                routeTransportCostParameterNames);
            return projection.fundsAfterTrade[region] -
                   state.S(fundsStockNames[region]);
        });
    }
    return model;
}

} // namespace Economic
} // namespace Civilization
} // namespace StonePotatoes
