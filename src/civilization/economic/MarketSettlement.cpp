// SPDX-License-Identifier: MIT
// Copyright (c) 2026 ArchaonHW
#include "MarketSettlement.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <stdexcept>
#include <utility>

namespace StonePotatoes {
namespace Civilization {
namespace Economic {
namespace {

using SellerCommodity = std::pair<std::size_t, std::string>;

bool IsFiniteNonnegative(float value) {
    return std::isfinite(value) && value >= 0.0f;
}

} // namespace

MarketSettlementResult SettleCommodityTrades(
    const std::vector<MarketAccount>& openingAccounts,
    const std::vector<CommodityTradeOrder>& orders) {
    if (openingAccounts.empty()) {
        throw std::invalid_argument("market settlement requires accounts");
    }
    for (const MarketAccount& account : openingAccounts) {
        if (!IsFiniteNonnegative(account.funds)) {
            throw std::invalid_argument("account funds must be finite and nonnegative");
        }
        for (const auto& [commodity, quantity] : account.inventory) {
            if (commodity.empty() || !IsFiniteNonnegative(quantity)) {
                throw std::invalid_argument(
                    "inventory names and quantities must be valid");
            }
        }
    }
    for (const CommodityTradeOrder& order : orders) {
        if (order.seller >= openingAccounts.size() ||
            order.buyer >= openingAccounts.size() ||
            order.seller == order.buyer || order.commodity.empty() ||
            !IsFiniteNonnegative(order.quantity) ||
            !IsFiniteNonnegative(order.unitPrice)) {
            throw std::invalid_argument("invalid commodity trade order");
        }
    }

    std::vector<double> accepted(orders.size(), 0.0);
    std::map<SellerCommodity, std::vector<std::size_t>> bySellerCommodity;
    for (std::size_t i = 0; i < orders.size(); ++i) {
        bySellerCommodity[{orders[i].seller, orders[i].commodity}].push_back(i);
        accepted[i] = orders[i].quantity;
    }
    for (const auto& [key, orderIndices] : bySellerCommodity) {
        const auto& inventory = openingAccounts[key.first].inventory;
        const auto stock = inventory.find(key.second);
        const double available = stock == inventory.end() ? 0.0 : stock->second;
        double requested = 0.0;
        for (const std::size_t index : orderIndices) {
            requested += accepted[index];
        }
        if (requested > available && requested > 0.0) {
            const double scale = available / requested;
            for (const std::size_t index : orderIndices) {
                accepted[index] *= scale;
            }
        }
    }

    std::vector<double> requestedPayments(openingAccounts.size(), 0.0);
    for (std::size_t i = 0; i < orders.size(); ++i) {
        requestedPayments[orders[i].buyer] +=
            accepted[i] * orders[i].unitPrice;
    }
    std::vector<double> buyerScale(openingAccounts.size(), 1.0);
    for (std::size_t buyer = 0; buyer < openingAccounts.size(); ++buyer) {
        if (requestedPayments[buyer] > openingAccounts[buyer].funds &&
            requestedPayments[buyer] > 0.0) {
            buyerScale[buyer] = openingAccounts[buyer].funds /
                               requestedPayments[buyer];
        }
    }

    MarketSettlementResult result{openingAccounts,
                                  std::vector<SettledCommodityTrade>(orders.size())};
    std::vector<double> debits(openingAccounts.size(), 0.0);
    std::vector<double> credits(openingAccounts.size(), 0.0);
    std::vector<std::map<std::string, double>> inventoryDeltas(
        openingAccounts.size());
    for (std::size_t i = 0; i < orders.size(); ++i) {
        const CommodityTradeOrder& order = orders[i];
        const double quantity = accepted[i] * buyerScale[order.buyer];
        const double payment = quantity * order.unitPrice;
        if (!std::isfinite(quantity) || !std::isfinite(payment) ||
            payment > std::numeric_limits<float>::max()) {
            throw std::overflow_error("settled trade exceeds supported range");
        }
        result.trades[i] = {static_cast<float>(quantity),
                            static_cast<float>(payment)};
        inventoryDeltas[order.seller][order.commodity] -= quantity;
        inventoryDeltas[order.buyer][order.commodity] += quantity;
        debits[order.buyer] += payment;
        credits[order.seller] += payment;
    }

    for (std::size_t accountIndex = 0;
         accountIndex < result.accounts.size(); ++accountIndex) {
        MarketAccount& account = result.accounts[accountIndex];
        const double funds = static_cast<double>(account.funds) -
                             debits[accountIndex] + credits[accountIndex];
        if (!std::isfinite(funds) || funds < -1e-6 ||
            funds > std::numeric_limits<float>::max()) {
            throw std::overflow_error("settled account funds exceed supported range");
        }
        account.funds = static_cast<float>(std::max(0.0, funds));
        for (const auto& [commodity, delta] : inventoryDeltas[accountIndex]) {
            const auto existing = account.inventory.find(commodity);
            const double initial = existing == account.inventory.end()
                ? 0.0 : existing->second;
            const double quantity = initial + delta;
            if (!std::isfinite(quantity) || quantity < -1e-6 ||
                quantity > std::numeric_limits<float>::max()) {
                throw std::overflow_error(
                    "settled inventory exceeds supported range");
            }
            account.inventory[commodity] =
                static_cast<float>(std::max(0.0, quantity));
        }
    }
    return result;
}

} // namespace Economic
} // namespace Civilization
} // namespace StonePotatoes
