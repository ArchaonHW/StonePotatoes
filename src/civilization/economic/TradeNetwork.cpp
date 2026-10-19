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
};

TradeStepProjection ProjectTradeStep(
    const SDState& state,
    const std::vector<std::string>& stockNames,
    const std::vector<std::string>& productionParameterNames,
    const std::vector<std::string>& consumptionParameterNames,
    const std::vector<float>& capacities,
    const std::vector<std::vector<std::string>>& routeParameterNames) {
    const std::size_t regionCount = stockNames.size();
    TradeStepProjection result{
        std::vector<float>(regionCount, 0.0f),
        std::vector<float>(regionCount, 0.0f)};
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
        for (std::size_t destination = 0; destination < regionCount;
             ++destination) {
            const float transfer = requestedTransfers[source][destination] *
                destinationScales[destination];
            result.tradeDelta[source] -= transfer;
            result.tradeDelta[destination] += transfer;
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
        for (std::size_t destination = 0; destination < regionCount;
             ++destination) {
            const std::string route =
                TradeRouteParameterName(region, destination);
            routeParameterNames[region][destination] = route;
            model.SetParam(
                route,
                config.monthlyTradeFromRegionToRegion[region][destination]);
        }
    }

    for (std::size_t region = 0; region < regionCount; ++region) {
        model.AddFlow(stockNames[region], [region, stockNames,
            productionParameterNames, consumptionParameterNames,
            capacities = config.storageCapacityByRegion,
            routeParameterNames](const SDState& state) {
            const TradeStepProjection projection = ProjectTradeStep(
                state, stockNames, productionParameterNames,
                consumptionParameterNames, capacities, routeParameterNames);
            return projection.stockAfterProductionAndConsumption[region] -
                       state.S(stockNames[region]) +
                   projection.tradeDelta[region];
        });
    }
    return model;
}

} // namespace Economic
} // namespace Civilization
} // namespace StonePotatoes
