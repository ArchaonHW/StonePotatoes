// SPDX-License-Identifier: MIT
// Copyright (c) 2026 ArchaonHW
#pragma once

#include <cstddef>
#include <map>
#include <string>
#include <vector>

namespace StonePotatoes {
namespace Civilization {
namespace Economic {

/** Inventory and spendable currency held by one market participant. */
struct MarketAccount {
    std::map<std::string, float> inventory;
    float funds = 0.0f;
};

/** Proposed transfer of one commodity from a seller to a buyer. */
struct CommodityTradeOrder {
    std::size_t seller = 0;
    std::size_t buyer = 0;
    std::string commodity;
    float quantity = 0.0f;
    float unitPrice = 0.0f;
};

struct SettledCommodityTrade {
    float quantity = 0.0f;
    float payment = 0.0f;
};

struct MarketSettlementResult {
    std::vector<MarketAccount> accounts;
    // Same order and size as the submitted order list.
    std::vector<SettledCommodityTrade> trades;
};

/**
 * Settle a batch of multi-commodity trades against opening balances.
 * Seller inventory limits are applied per seller and commodity, then buyer
 * funds limits are applied across all incoming orders. Both constraints scale
 * affected orders proportionally rather than granting priority by order-list
 * position.
 * Sales proceeds become available after the batch and cannot fund purchases
 * within the same batch. Invalid input throws without mutating caller state.
 */
MarketSettlementResult SettleCommodityTrades(
    const std::vector<MarketAccount>& openingAccounts,
    const std::vector<CommodityTradeOrder>& orders);

} // namespace Economic
} // namespace Civilization
} // namespace StonePotatoes
