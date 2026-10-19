// SPDX-License-Identifier: MIT
// Copyright (c) 2026 ArchaonHW
#pragma once

#include "civilization/economic/SystemDynamics.h"

namespace StonePotatoes {
namespace Civilization {
namespace Economic {

/** Configuration for one stock-sensitive, monthly commodity market price. */
struct MarketPriceConfig {
    float initialPrice = 1.0f;
    float targetStock = 100.0f;
    // Fractional price response per month when stock is maximally scarce/abundant.
    float responsePerMonth = 0.1f;
    float minimumPrice = 0.01f;
    float maximumPrice = 1000000.0f;
};

/**
 * Build a bounded market price model with an externally supplied inventory.
 * Each step adjusts price toward higher values below target stock and lower
 * values above it. Inventory is a runtime parameter so this model can consume
 * a stock from TradeNetwork without duplicating or changing that model.
 */
SDModel BuildMarketPriceModel(const MarketPriceConfig& config);

const char* MarketPriceStockName();
const char* MarketStockParameterName();
const char* MarketTargetStockParameterName();
const char* MarketResponseParameterName();

} // namespace Economic
} // namespace Civilization
} // namespace StonePotatoes
