// SPDX-License-Identifier: MIT
// Copyright (c) 2026 ArchaonHW
#include "civilization/economic/MarketPrice.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace StonePotatoes {
namespace Civilization {
namespace Economic {

const char* MarketPriceStockName() { return "market_price"; }
const char* MarketStockParameterName() { return "market_stock"; }
const char* MarketTargetStockParameterName() { return "market_target_stock"; }
const char* MarketResponseParameterName() { return "market_response_per_month"; }

SDModel BuildMarketPriceModel(const MarketPriceConfig& config) {
    if (!std::isfinite(config.initialPrice) ||
        !std::isfinite(config.targetStock) || config.targetStock <= 0.0f ||
        !std::isfinite(config.responsePerMonth) ||
        config.responsePerMonth < 0.0f || config.responsePerMonth > 1.0f ||
        !std::isfinite(config.minimumPrice) || config.minimumPrice < 0.0f ||
        !std::isfinite(config.maximumPrice) ||
        config.maximumPrice <= config.minimumPrice ||
        config.initialPrice < config.minimumPrice ||
        config.initialPrice > config.maximumPrice) {
        throw std::invalid_argument("invalid market price configuration");
    }

    SDModel model;
    model.AddStock(MarketPriceStockName(), config.initialPrice,
                   config.minimumPrice, config.maximumPrice);
    model.SetParam(MarketStockParameterName(), config.targetStock);
    model.SetParam(MarketTargetStockParameterName(), config.targetStock);
    model.SetParam(MarketResponseParameterName(), config.responsePerMonth);
    model.AddFlow(MarketPriceStockName(), [] (const SDState& state) {
        const float target = state.P(MarketTargetStockParameterName());
        const float stock = state.P(MarketStockParameterName());
        const float rawResponse = state.P(MarketResponseParameterName());
        if (!std::isfinite(target) || target <= 0.0f ||
            !std::isfinite(stock) || !std::isfinite(rawResponse)) {
            return 0.0f;
        }

        const float response = std::clamp(rawResponse, 0.0f, 1.0f);
        const float inventoryRatio = std::max(0.0f, stock) / target;
        const float scarcity = std::clamp(1.0f - inventoryRatio,
                                          -1.0f, 1.0f);
        return state.S(MarketPriceStockName()) * response * scarcity;
    });
    return model;
}

} // namespace Economic
} // namespace Civilization
} // namespace StonePotatoes
