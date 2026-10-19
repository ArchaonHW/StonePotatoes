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
        routeTransportCostParameterNames,
    const std::vector<std::vector<std::string>>&
        routeTransactionPriceParameterNames) {
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

    std::vector<float> fundsScaleBySource(regionCount, 1.0f);
    std::vector<float> availableFundsByRegion(regionCount, 0.0f);
    std::vector<float> estimatedTransportCostByRegion(regionCount, 0.0f);
    for (std::size_t source = 0; source < regionCount; ++source) {
        if (!fundsStockNames.empty()) {
            const float rawIncome =
                state.P(fundsIncomeParameterNames[source]);
            const float income = std::isfinite(rawIncome)
                ? std::max(0.0f, rawIncome) : 0.0f;
            availableFundsByRegion[source] = std::max(
                0.0f, state.S(fundsStockNames[source])) + income;

            if (!routeTransportCostParameterNames.empty()) {
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
                if (requestedTransportCost > availableFundsByRegion[source] &&
                    requestedTransportCost > 0.0f) {
                    fundsScaleBySource[source] =
                        availableFundsByRegion[source] / requestedTransportCost;
                }
                estimatedTransportCostByRegion[source] =
                    requestedTransportCost * fundsScaleBySource[source];
            }
        }
    }

    std::vector<float> buyerScaleByDestination(regionCount, 1.0f);
    if (!routeTransactionPriceParameterNames.empty()) {
        for (std::size_t destination = 0; destination < regionCount;
             ++destination) {
            float requestedPayment = 0.0f;
            for (std::size_t source = 0; source < regionCount; ++source) {
                const float rawPrice = state.P(
                    routeTransactionPriceParameterNames[source][destination]);
                const float unitPrice = std::isfinite(rawPrice)
                    ? std::max(0.0f, rawPrice) : 0.0f;
                requestedPayment +=
                    requestedTransfers[source][destination] *
                    destinationScales[destination] *
                    fundsScaleBySource[source] * unitPrice;
            }
            const float spendableFunds = std::max(
                0.0f, availableFundsByRegion[destination] -
                          estimatedTransportCostByRegion[destination]);
            if (requestedPayment > spendableFunds &&
                requestedPayment > 0.0f) {
                buyerScaleByDestination[destination] =
                    spendableFunds / requestedPayment;
            }
        }
    }

    std::vector<float> actualTransportCostByRegion(regionCount, 0.0f);
    std::vector<float> salesByRegion(regionCount, 0.0f);
    std::vector<float> purchasesByRegion(regionCount, 0.0f);
    for (std::size_t source = 0; source < regionCount; ++source) {
        for (std::size_t destination = 0; destination < regionCount;
             ++destination) {
            const float transfer = requestedTransfers[source][destination] *
                destinationScales[destination] *
                fundsScaleBySource[source] *
                buyerScaleByDestination[destination];
            if (!routeTransportCostParameterNames.empty()) {
                const float rawCost = state.P(
                    routeTransportCostParameterNames[source][destination]);
                const float unitCost = std::isfinite(rawCost)
                    ? std::max(0.0f, rawCost) : 0.0f;
                actualTransportCostByRegion[source] += transfer * unitCost;
            }
            if (!routeTransactionPriceParameterNames.empty()) {
                const float rawPrice = state.P(
                    routeTransactionPriceParameterNames[source][destination]);
                const float unitPrice = std::isfinite(rawPrice)
                    ? std::max(0.0f, rawPrice) : 0.0f;
                const float payment = transfer * unitPrice;
                salesByRegion[source] += payment;
                purchasesByRegion[destination] += payment;
            }
            result.tradeDelta[source] -= transfer;
            result.tradeDelta[destination] += transfer;
        }
    }
    for (std::size_t region = 0; region < fundsStockNames.size(); ++region) {
        result.fundsAfterTrade[region] = std::max(
            0.0f, availableFundsByRegion[region] -
                      actualTransportCostByRegion[region] +
                      salesByRegion[region] - purchasesByRegion[region]);
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

std::string TradeRouteTransactionPriceParameterName(
    std::size_t fromRegionIndex, std::size_t toRegionIndex) {
    return IndexedName("trade_transaction_price_per_unit_region_",
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
    const bool hasTransactionPrices =
        !config.monthlyTransactionPricePerUnitFromRegionToRegion.empty();
    const bool hasFunds = hasTransportCosts || hasTransactionPrices;
    if ((!hasFunds && !config.initialFundsByRegion.empty()) ||
        (!hasFunds && !config.monthlyFundsIncomeByRegion.empty()) ||
        (hasFunds &&
         config.initialFundsByRegion.size() != regionCount) ||
        (!config.monthlyFundsIncomeByRegion.empty() &&
         config.monthlyFundsIncomeByRegion.size() != regionCount) ||
        (hasTransportCosts &&
         config.monthlyTransportCostPerUnitFromRegionToRegion.size() !=
             regionCount) ||
        (hasTransactionPrices &&
         config.monthlyTransactionPricePerUnitFromRegionToRegion.size() !=
             regionCount)) {
        throw std::invalid_argument(
            "trade funds and route pricing vectors must match regions");
    }

    for (std::size_t region = 0; region < regionCount; ++region) {
        if (hasFunds) {
            const float funds = config.initialFundsByRegion[region];
            const float income = config.monthlyFundsIncomeByRegion.empty()
                ? 0.0f : config.monthlyFundsIncomeByRegion[region];
            if (!std::isfinite(funds) || funds < 0.0f ||
                !std::isfinite(income) || income < 0.0f) {
                throw std::invalid_argument(
                    "invalid trade funds or monthly income");
            }
            if (hasTransportCosts) {
                if (config.monthlyTransportCostPerUnitFromRegionToRegion[region]
                        .size() != regionCount) {
                    throw std::invalid_argument(
                        "transport cost matrix must be square");
                }
                for (std::size_t destination = 0; destination < regionCount;
                     ++destination) {
                    const float cost =
                        config.monthlyTransportCostPerUnitFromRegionToRegion
                            [region][destination];
                    if (!std::isfinite(cost) || cost < 0.0f ||
                        (region == destination && cost != 0.0f)) {
                        throw std::invalid_argument(
                            "transport costs must be finite, nonnegative, "
                            "and exclude self-routes");
                    }
                }
            }
            if (hasTransactionPrices) {
                if (config.monthlyTransactionPricePerUnitFromRegionToRegion
                        [region].size() != regionCount) {
                    throw std::invalid_argument(
                        "transaction price matrix must be square");
                }
                for (std::size_t destination = 0; destination < regionCount;
                     ++destination) {
                    const float price =
                        config.monthlyTransactionPricePerUnitFromRegionToRegion
                            [region][destination];
                    if (!std::isfinite(price) || price < 0.0f ||
                        (region == destination && price != 0.0f)) {
                        throw std::invalid_argument(
                            "transaction prices must be finite, "
                            "nonnegative, and exclude self-routes");
                    }
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
    std::vector<std::vector<std::string>> routeTransactionPriceParameterNames;
    if (hasTransportCosts) {
        routeTransportCostParameterNames.assign(
            regionCount, std::vector<std::string>(regionCount));
    }
    if (hasTransactionPrices) {
        routeTransactionPriceParameterNames.assign(
            regionCount, std::vector<std::string>(regionCount));
    }
    if (hasFunds) {
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
        if (hasFunds) {
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
            if (hasTransactionPrices) {
                const std::string price =
                    TradeRouteTransactionPriceParameterName(region,
                                                            destination);
                routeTransactionPriceParameterNames[region][destination] =
                    price;
                model.SetParam(price,
                    config.monthlyTransactionPricePerUnitFromRegionToRegion
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
            routeTransportCostParameterNames,
            routeTransactionPriceParameterNames](const SDState& state) {
            const TradeStepProjection projection = ProjectTradeStep(
                state, stockNames, productionParameterNames,
                consumptionParameterNames, capacities, routeParameterNames,
                fundsStockNames, fundsIncomeParameterNames,
                routeTransportCostParameterNames,
                routeTransactionPriceParameterNames);
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
            routeTransportCostParameterNames,
            routeTransactionPriceParameterNames](const SDState& state) {
            const TradeStepProjection projection = ProjectTradeStep(
                state, stockNames, productionParameterNames,
                consumptionParameterNames, capacities, routeParameterNames,
                fundsStockNames, fundsIncomeParameterNames,
                routeTransportCostParameterNames,
                routeTransactionPriceParameterNames);
            return projection.fundsAfterTrade[region] -
                   state.S(fundsStockNames[region]);
        });
    }
    return model;
}

} // namespace Economic
} // namespace Civilization
} // namespace StonePotatoes
